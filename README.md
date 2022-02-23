# crown
1. При использовании Open Sound System (OSS) в UNIX-подобных системах дополнительно необходимо установить osspd (по умолчанию, sudo apt-get install osspd). Признак того, что драйвер не установлен:

ad_oss.c(115): Failed to open audio device(/dev/dsp): No such file or directory
FATAL: "continuous.c", line 50: Failed to open audio device
