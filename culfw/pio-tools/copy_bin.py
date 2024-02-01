Import("env")
import shutil

#print(env.Dump())

APP_BIN = "$BUILD_DIR/${PROGNAME}.hex"
OTA_BIN = "/share/GIT/busware-esp32/firmware/${PIOENV}.hex"
BOARD_CONFIG = env.BoardConfig()


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
env.AddPostAction(APP_BIN, bin_map_copy)

