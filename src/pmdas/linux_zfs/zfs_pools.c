#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include "pmapi.h"
#include "libpcp.h"
#include "pmda.h"

#include "zfs_utils.h"
#include "zfs_pools.h"

void
zfs_pools_init(zfs_poolstats_t **poolstats, pmdaInstid **pools, pmdaIndom *poolsindom)
{
    DIR *zfs_dp;
    struct dirent *ep;
    int pool_num = 0;
    size_t size;
    zfs_poolstats_t *poolstats_tmp;
    static int seen_err = 0;

    // Discover the pools by looking for directories in /proc/spl/kstat/zfs
    if ((zfs_dp = opendir(ZFS_PATH)) != NULL) {
        while ((ep = readdir(zfs_dp))) {
            if (ep->d_type == DT_DIR) {
                if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
                    continue;
                else {
                    pmdaInstid    *pools_tmp;
                    size = (pool_num + 1) * sizeof(pmdaInstid);
                    if ((pools_tmp = (pmdaInstid *)realloc(*pools, size)) == NULL)
                        pmNoMem("pools", size, PM_FATAL_ERR);
                    *pools = pools_tmp;
                    (*pools)[pool_num].i_name = (char *) malloc(strlen(ep->d_name) + 1);
                    strcpy((*pools)[pool_num].i_name, ep->d_name);
                    (*pools)[pool_num].i_name[strlen(ep->d_name)] = '\0';
                    (*pools)[pool_num].i_inst = pool_num;
                    pool_num++;
                }
            }
        }
        closedir(zfs_dp);
    }
    else {
        pmNotifyErr(LOG_WARNING, "Failed to open ZFS pools dir \"%s\": %s\n", ZFS_PATH, pmErrStr(-errno));
    }
    if (*pools == NULL) {
        if (! seen_err) {
            pmNotifyErr(LOG_WARNING, "no ZFS pools found, instance domain is empty.");
            seen_err = 1;
        }
    }
    else if (seen_err) {
        pmNotifyErr(LOG_INFO, "%d ZFS pools found.", pool_num);
        seen_err = 0;
    }
    (*poolsindom).it_set = *pools;
    (*poolsindom).it_numinst = pool_num;
    if (pool_num > 0) {
        if ((poolstats_tmp = (zfs_poolstats_t *)realloc(*poolstats, pool_num * sizeof(zfs_poolstats_t))) == NULL)
            pmNoMem("poolstats init", pool_num * sizeof(zfs_poolstats_t), PM_FATAL_ERR);
        *poolstats = poolstats_tmp;
    }
}

void
zfs_pools_clear(zfs_poolstats_t **poolstats, pmdaInstid **pools, pmdaIndom *poolsindom)
{
    int i;

    for (i = 0; i < (*poolsindom).it_numinst; i++) {
        free((*pools)[i].i_name);
        (*pools)[i].i_name = NULL;
    }
    if (*pools)
        free(*pools);
    if (*poolstats)
        free(*poolstats);
    *poolstats = NULL;
    (*poolsindom).it_set = *pools = NULL;
    (*poolsindom).it_numinst = 0;
}

void
zfs_poolstats_refresh(zfs_poolstats_t **poolstats, pmdaInstid **pools, pmdaIndom *poolsindom)
{
    int i, nread_seen;
    char pool_dir[MAXPATHLEN+64], fname[MAXPATHLEN+128];
    char *line = NULL, *token, delim[] = " ";
    FILE *fp;
    struct stat sstat;
    size_t len = 0;

    if (poolstats != NULL)
        zfs_pools_clear(poolstats, pools, poolsindom);
    zfs_pools_init(poolstats, pools, poolsindom);
    if (poolsindom->it_numinst == 0) {
        /* no pools, nothing to do */
        return;
    }

    if ((*poolstats = realloc(*poolstats, (*poolsindom).it_numinst * sizeof(zfs_poolstats_t))) == NULL)
        pmNoMem("poolstats refresh", (*poolsindom).it_numinst * sizeof(zfs_poolstats_t), PM_FATAL_ERR);
    for (i = 0; i < (*poolsindom).it_numinst; i++) {
        pool_dir[0] = 0;        
        sprintf(pool_dir, "%s%c%s", ZFS_PATH, pmPathSeparator(), (*poolsindom).it_set[i].i_name);
        if (stat(pool_dir, &sstat) != 0) {
            continue;
        }
        // Read the state if exists
        (*poolstats)[i].state = 13; // UNKNOWN
        fname[0] = 0;
        sprintf(fname, "%s%c%s", pool_dir, pmPathSeparator(), "state");
        fp = fopen(fname, "r");
        if (fp != NULL) {
            while (getline(&line, &len, fp) != -1) {
                if (strncmp(line, "OFFLINE", 7) == 0) (*poolstats)[i].state = 0;
                else if (strncmp(line, "ONLINE", 6) == 0) (*poolstats)[i].state = 1;
                else if (strncmp(line, "DEGRADED", 8) == 0) (*poolstats)[i].state = 2;
                else if (strncmp(line, "FAULTED", 7) == 0) (*poolstats)[i].state = 3;
                else if (strncmp(line, "REMOVED", 7) == 0) (*poolstats)[i].state = 4;
                else if (strncmp(line, "UNAVAIL", 7) == 0) (*poolstats)[i].state = 5;
            }
            fclose(fp);
        }
        // Read the IO stats
        fname[0] = 0;
        sprintf(fname, "%s%c%s", pool_dir, pmPathSeparator(), "io");
        fp = fopen(fname, "r");
        if (fp != NULL) {
            nread_seen = 0;
            while (getline(&line, &len, fp) != -1) {
                if (nread_seen == 1) {
                    // Tokenize the line to extract the metrics
                    (*poolstats)[i].nread        = strtoul(strtok(line, delim), NULL, 0);
                    (*poolstats)[i].nwritten        = strtoul(strtok(NULL, delim), NULL, 0);
                    (*poolstats)[i].reads        = strtoul(strtok(NULL, delim), NULL, 0);
                    (*poolstats)[i].writes        = strtoul(strtok(NULL, delim), NULL, 0);
                    (*poolstats)[i].wtime        = strtoul(strtok(NULL, delim), NULL, 0);
                    (*poolstats)[i].wlentime        = strtoul(strtok(NULL, delim), NULL, 0);
                    (*poolstats)[i].wupdate        = strtoul(strtok(NULL, delim), NULL, 0);
                    (*poolstats)[i].rtime        = strtoul(strtok(NULL, delim), NULL, 0);
                    (*poolstats)[i].rlentime        = strtoul(strtok(NULL, delim), NULL, 0);
                    (*poolstats)[i].rupdate        = strtoul(strtok(NULL, delim), NULL, 0);
                    (*poolstats)[i].wcnt        = strtoul(strtok(NULL, delim), NULL, 0);
                    (*poolstats)[i].rcnt        = strtoul(strtok(NULL, delim), NULL, 0);
                }
                else {
                    // Search for the header line
                    token = strtok(line, delim);
                    if (strcmp(token, "nread"))
                        nread_seen++;
                }
            }
            fclose(fp);
        }
    }
    if (line != NULL)
        free(line);
}
