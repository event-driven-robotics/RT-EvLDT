# This shell script can be used to download udev rules and reload udev rules automatically in host environment.

#!/bin/bash

cd /etc/udev/rules.d
if [ -f "88-cyusb.rules" ]; then
    echo "88-cyusb.rules exists"
else
    echo "No 88-cyusb.rules exists"
    sudo wget https://raw.githubusercontent.com/prophesee-ai/openeb/main/hal_psee_plugins/resources/rules/88-cyusb.rules
fi
if [ -f "99-evkv2.rules" ]; then
    echo "99-evkv2.rules exists"
else
    echo "No 99-evkv2.rules exists"
    sudo wget https://raw.githubusercontent.com/prophesee-ai/openeb/main/hal_psee_plugins/resources/rules/99-evkv2.rules
fi

# reload udev rules
echo "Reloading udev rules..."
sudo udevadm control --reload-rules
sudo udevadm trigger

echo "Setup complete. Please connect your USB device to verify the permissions change."