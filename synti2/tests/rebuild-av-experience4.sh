die()
{
    echo Error: $1
    exit 1
}

killall jackvsynti2
make product/ || die "Build of hack3 failed"
cd product/
make bin/jackvsynti2 || die "Build of vis2 failed"
cd ..
product/bin/jackvsynti2 &
sleep 1 && \
  aj-snapshot -r local_hacks/aj-s4
