{
    "targets": [
        {
            "target_name": "blue",
            "cflags!": [ 
                "-fno-exceptions"
            ],
            "cflags_cc!": [ 
                "-fno-exceptions" 
            ],
            "sources": [
                "main.cc"
            ],
            "libraries": [
                "/usr/lib/arm-linux-gnueabihf/libbluetooth.so"
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")"
            ]
        }
    ]
}