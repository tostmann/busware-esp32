Import("env")
import shutil
import os

#print(env.Dump())

APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"
LFS_BIN  = "$BUILD_DIR/littlefs.bin"
SFS_BIN  = "$BUILD_DIR/spiffs.bin"
#MERGED_BIN = "$BUILD_DIR/${PROGNAME}.factory.bin"
MERGED_BIN = "/share/GIT/busware-esp32/firmware/${PIOENV}.factory.bin"
OTA_BIN = "/share/GIT/busware-esp32/firmware/${PIOENV}.ota.bin"
BOARD_CONFIG = env.BoardConfig()


def merge_bin(source, target, env):
    # The list contains all extra images (bootloader, partitions, eboot) and
    # the final application binary
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["$ESP32_APP_OFFSET", APP_BIN]

    if os.path.isfile( env.get("PROJECT_BUILD_DIR")+"/"+env.get("PIOENV")+"/littlefs.bin" ):
        flash_images += ["0x290000", LFS_BIN]
         
    if os.path.isfile( env.get("PROJECT_BUILD_DIR")+"/"+env.get("PIOENV")+"/spiffs.bin" ):
        flash_images += ["0x290000", SFS_BIN]
         
    # Run esptool to merge images into a single binary
    env.Execute(
        " ".join(
            [
                "$PYTHONEXE",
                "$OBJCOPY",
                "--chip",
                BOARD_CONFIG.get("build.mcu", "esp32"),
                "merge_bin",
#                "--fill-flash-size",
#                BOARD_CONFIG.get("upload.flash_size", "4MB"),
                "--output",
                MERGED_BIN,
            ]
            + flash_images
        )
    )

def bin_map_copy(source, target, env):
    env.Execute(
        " ".join(
            [
                "cp",
                APP_BIN,
                OTA_BIN
            ]
        )
    )
# Add a post action that runs esptoolpy to merge available flash images
env.AddPostAction(APP_BIN, merge_bin)
env.AddPostAction(APP_BIN, bin_map_copy)

# Patch the upload command to flash the merged binary at address 0x0
env.Replace(
    UPLOADERFLAGS=[
        ]
        + ["0x0", MERGED_BIN],
    UPLOADCMD='"$PYTHONEXE" "$UPLOADER" $UPLOADERFLAGS',
)
