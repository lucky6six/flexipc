

proc boot { } {
    expect {
        "uart init finished" { }
        timeout { error "uart init failed" }
    }
    expect {
        "interrupt init finished" { }
        timeout { error "interrupt init failed" }
    }
    expect {
        "mm init finished" { }
        timeout { error "mm init failed" }
    }
    expect {
        "lock init finished" { }
        timeout { error "lock init failed" }
    }
    expect {
        "sched init finished" { }
        timeout { error "schd init failed" }
    }
    expect {
        "create initial thread done" { }
        timeout { error "create init_thread failed" }
    }
}


proc boot_hikey { } {
    expect {
        "Flashing partition system" {
            exec sudo screen -x -S hikey -p 0 -X stuff "\r\n"
            sleep 1
        }
        timeout { error "failed flashing system partition" }
    }
    expect {
        "create initial thread done" { }
        timeout { error "create initial thread failed" }
    }
}
