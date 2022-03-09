vr_reg_loc=$(find ~ -iname vrpathreg)
vr_reg_loc_dir="$(dirname "${vr_reg_loc}")"
echo "vrpathreg: ${vr_reg_loc}"
echo "libopenvr.so path: ${vr_reg_loc_dir}"
hobo_dir=""
true_hobo_dir="$(cd "$(dirname "$hobo_dir")"; pwd)/$(basename "$hobo_dir")"


LD_LIBRARY_PATH="${vr_reg_loc_dir}" $vr_reg_loc adddriver $true_hobo_dir

LD_LIBRARY_PATH="${vr_reg_loc_dir}" $vr_reg_loc show