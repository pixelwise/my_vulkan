
set -e
pip3 install --upgrade --user -r requirements_dev.txt
pkgs="$(jinja2 pkglist_dev.txt -D VERSION_ID='$(lsb_release -r -s)' | tr '\n' ' ')"
# we do not want the source and jinja2 run with sudo
sudo dnf -y install ${pkgs}
git clean -xdf
