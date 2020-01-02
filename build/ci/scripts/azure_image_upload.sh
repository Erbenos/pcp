#!/bin/sh -eux
image_raw="build-${AZ_IMAGE}/${AZ_IMAGE}.img"
image_vhd="build-${AZ_IMAGE}/${AZ_IMAGE}.vhd"
image_vhd_url="${AZ_STORAGE_URL}/${AZ_STORAGE_CONTAINER}/${AZ_IMAGE}.vhd"

# Azure VM images need to be aligned at 1MB
MB=$((1024*1024))
size=$(qemu-img info -f raw --output json "${image_raw}" | jq '."virtual-size"')
rounded_size=$((($size/$MB + 1)*$MB))
qemu-img resize -f raw "${image_raw}" ${rounded_size}

qemu-img convert -f raw -o subformat=fixed,force_size -O vpc "${image_raw}" "${image_vhd}"
azcopy copy "${image_vhd}" "${image_vhd_url}"

az image create \
  --resource-group "${AZ_RESOURCE_GROUP}" \
  --name "${AZ_IMAGE}" \
  --os-type Linux \
  --source "${image_vhd_url}"

az storage blob delete \
  --account-name "${AZ_STORAGE_ACCOUNT}" \
  --container "${AZ_STORAGE_CONTAINER}" \
  --name "${AZ_IMAGE}.vhd"
