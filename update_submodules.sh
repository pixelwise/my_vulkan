for i in $(seq 1 5); do
    [ $i -gt 1 ] && sleep 1
    git submodule sync && git submodule update --init --recursive && s=0 && break || s=$?
    echo retrying...
done;
(exit $s)
