set -e
env_paths=(
"/opt/gameon/gdenv"
)
for env_path in "${env_paths[@]}"
do
    if [ -e "${env_path}" ]
    then
        echo "source ${env_path}"
        source "${env_path}"
    fi
done
if [ -z "${VIRTUAL_ENV}" ]
then
    pip_opts="--user"
fi
pip3 install ${pip_opts} --upgrade -r prerequirements_dev.txt
pkgs="$(jinja2 pkglist_dev.txt -D VERSION_ID='$(lsb_release -r -s)' | tr '\n' ' ')"
# we do not want the source and jinja2 run with sudo
sudo dnf -y install ${pkgs}
pip3 install --upgrade -r requirements_dev.txt
git clean -xdf
