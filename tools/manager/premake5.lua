workspace "Penguin"
    configurations { "Debug", "Release" }

    configurations "Debug"
		symbols "On"

    configurations "Release"
		optimize "On"

    language "C"
    warnings "Extra"
    buildoptions { "-idirafter ../../../kernel/src", "-DLINUX -std=gnu99", "-mcmodel=large", "-Wno-unused", "-Wno-format", "-Wno-unused-result", "-fno-builtin" }
    linkoptions { "-nostartfiles", "-Wl,-Ttext-segment=0xff00000000" }

    filter 'files:src/**.asm'
        buildmessage 'Compiling %{file.relpath}'
        buildcommands {
            'nasm -f elf64 -o "%{cfg.objdir}/%{file.basename}.o" "%{file.relpath}"'
        }
        buildoutputs { '%{cfg.objdir}/%{file.basename}.o' }

    project "Manager"

        kind "ConsoleApp"
        location "build"
        targetname "manager"
        targetdir "."
        files { "src/**.h", "src/**.c", "src/**.asm" }
        includedirs { "..", "../../lib/include" }
        libdirs { ".", "build" }
        links { "ext", "rt", "vnic", "tlsf" }

        -- Make version header
        prebuildcommands {
            '../mkver.sh > ../src/version.h'
        }

		-- Copy libraries into the build directory
        prelinkcommands {
            'cp ../../../lib/libext.a .',
            'cp ../../../lib/libvnic.a .',
            'cp ../../../lib/libtlsf.a .'
        }

