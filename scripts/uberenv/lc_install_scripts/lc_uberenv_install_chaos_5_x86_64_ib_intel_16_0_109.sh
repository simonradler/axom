#!/bin/bash
###############################################################################
# use uberenv to build third party libs on chaos 5 using intel 16
###############################################################################
source setup_dir.sh
python ../uberenv.py --prefix $LC_TPL_PATH --spec %intel@16.0.109
# set the proper permissions
./lc_install_set_perms.sh

