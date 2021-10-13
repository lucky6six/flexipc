cmake_minimum_required(VERSION 3.11)

add_executable(ufs.srv
    ufs.c
    ufshci.c
    ufsutp.c
)

add_executable(ufs-linux.srv
    ./linux-port/main.c
    ./linux-port/ufshcd.c
)
