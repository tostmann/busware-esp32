#!/bin/bash

cd /share/SVN/culfw-code/culfw/Devices/nanoCUL/

rm -fr /share/GIT/busware-esp32/culfw/lib
mkdir -p /share/GIT/busware-esp32/culfw/lib/clib
mkdir -p /share/GIT/busware-esp32/culfw/lib/mbus

rm -fr /share/GIT/busware-esp32/culfw/include
mkdir /share/GIT/busware-esp32/culfw/include
cp board.h /share/GIT/busware-esp32/culfw/include
cp ../../clib/led.h \
	../../clib/battery.h \
	../../clib/joy.h \
	../../clib/i2cmaster.h \
	../../clib/adcw.h \
	../../clib/cctemp.h \
	../../clib/fswrapper.h \
	../../clib/qfs.h \
	../../clib/df.h \
	../../clib/pcf8833.h \
	../../clib/log.h \
	../../clib/mysleep.h \
	../../clib/ntp.h \
	../../clib/ethernet.h \
	../../clib/rf_router.h /share/GIT/busware-esp32/culfw/include

cp ../../version.h /share/GIT/busware-esp32/culfw/lib/

cp ../../clib/cc1100.*                                \
                ../../clib/cc1101_pllcheck.*                                  \
                ../../clib/clock.*                                   \
                ../../clib/delay.*                                   \
                ../../clib/display.*                                 \
                ../../clib/stringfunc.*                              \
                ../../clib/fncollection.*                            \
                ../../clib/ringbuffer.*                              \
                ../../clib/fht.*                                     \
                ../../clib/rf_send.*                                 \
                ../../clib/rf_receive.*                              \
                ../../clib/rf_asksin.*                               \
                ../../clib/rf_moritz.*                               \
                ../../clib/rf_rwe.*                               \
                ../../clib/somfy_rts.*                               \
                ../../clib/fastrf.*                               \
                ../../clib/rf_zwave.*                                \
                ../../clib/intertechno.*                             \
                ../../clib/kopp-fc.*                                 \
                ../../clib/memory.*                                  \
                ../../clib/serial.*                                  \
                ../../clib/ttydata.*                                 \
                ../../clib/spi.*                                     \
                ../../clib/rf_mbus.*   /share/GIT/busware-esp32/culfw/lib/clib/ 

cp                ../../clib/mbus/manchester.*                         \
                ../../clib/mbus/3outof6.*                            \
                ../../clib/mbus/mbus_packet.*                        \
                ../../clib/mbus/crc.* /share/GIT/busware-esp32/culfw/lib/mbus/

rm -fr /share/GIT/busware-esp32/culfw/src
mkdir /share/GIT/busware-esp32/culfw/src
cp nanoCUL.c board.h /share/GIT/busware-esp32/culfw/src/

cd -

