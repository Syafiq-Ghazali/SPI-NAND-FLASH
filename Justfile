# Project-specific fields
project_name := "SPI_NAND"
build_dir := "CPU1_FLASH"
target_config := "targetConfigs/TMS320F280039C_LaunchPad.ccxml"

# dslite.bat must be run on CMD (doesn't work on anything else)
cmd_preamble := "cmd.exe /C"

# Paths to gmake and dslite
gmake := "/mnt/c/ti/ccs2020/ccs/utils/bin/gmake.exe"
dslite := "C:/ti/uniflash_9.2.0/dslite.bat"

build:
    {{gmake}} -C {{build_dir}} -k -j 16 all -O

clean:
    {{gmake}} -C {{build_dir}} -k -j 16 clean -O

rebuild: clean build

flash: build
    {{cmd_preamble}} "{{dslite}} -c {{target_config}} -f -v {{build_dir}}/{{project_name}}.out"
    {{cmd_preamble}} "{{dslite}} -c {{target_config}} -r 1"

