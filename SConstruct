import os
import multiprocessing
from pathlib import Path


def PhonyTargets(
    target,
    action,
    depends,
    env=None,
):
    # Creates a Phony target
    if not env:
        env = DefaultEnvironment()
    t = env.Alias(target, depends, action)
    env.AlwaysBuild(t)


SIMULATED_PROGRAM = "app"
TARGET_PROGRAM = "pressa-display-rotondi.bin"
LIBS = "lib"
MAIN = "src"
LVGL = f"{LIBS}/lvgl"


CFLAGS = [
    "-Wall",
    "-Wextra",
    "-Wno-unused-function",
    "-g",
    "-O0",
    "-DBUILD_CONFIG_DISPLAY_HORIZONTAL_RESOLUTION=800",
    "-DBUILD_CONFIG_DISPLAY_VERTICAL_RESOLUTION=480",
]

CPPPATH = [
    f"#{MAIN}", f"#{MAIN}/config", f"#{LVGL}"
]


def get_target(env, name, suffix="", dependencies=[]):
    def rchop(s, suffix):
        if suffix and s.endswith(suffix):
            return s[:-len(suffix)]
        return s

    sources = [File(filename)
               for filename in Path(f"{MAIN}/").rglob('*.c')]
    sources += [ File(filename) for filename in Path(f'{LVGL}/src').rglob('*.c') ]

    pman_env = env
    (lv_pman, include) = SConscript( f'{LIBS}/c-page-manager/SConscript', variant_dir=f"build-{name}/lv_pman", exports=['pman_env'])
    env['CPPPATH'] += [include]

    objects = [env.Object(
        f"{rchop(x.get_abspath(), '.c')}{suffix}", x) for x in sources]

    target = env.Program(name, objects + [lv_pman])
    env.Depends(target, dependencies)
    env.Clean(target, f"build-{name}")
    return target


def main():
    num_cpu = multiprocessing.cpu_count()
    SetOption('num_jobs', num_cpu)
    print("Running with -j {}".format(GetOption('num_jobs')))

    env_options = {
        "ENV": os.environ,
        "CPPPATH": CPPPATH,
        'CPPDEFINES': [],
        "CCFLAGS": CFLAGS + ["-DLV_USE_SDL", "-DBUILD_CONFIG_SIMULATOR"],
        "LIBS": ["-lpthread", "-lSDL2"]
    }

    simulated_env = Environment(**env_options)
    simulated_env.Tool('compilation_db')

    target_env = simulated_env.Clone(
        LIBS=["-lpthread", "-larchive"], CC="~/Mount/Data/Projects/new_buildroot/buildrpi3/output/host/bin/aarch64-buildroot-linux-uclibc-gcc",
        CCFLAGS=CFLAGS + ["-DUSE_FBDEV=1", "-DUSE_EVDEV=1"])

    simulated_prog = get_target(
        simulated_env, SIMULATED_PROGRAM, dependencies=[])
    target_prog = get_target(target_env, "pressa-display-rotondi", suffix="-pi", dependencies=[])

    PhonyTargets('run', f"./{SIMULATED_PROGRAM}",
                 simulated_prog, simulated_env)
    compileDB = simulated_env.CompilationDatabase('compile_commands.json')

    ip_addr = ARGUMENTS.get("ip", "")
    compatibility_options = "-o StrictHostKeyChecking=no -o PubkeyAcceptedAlgorithms=+ssh-rsa"
    PhonyTargets(
        'kill-remote',
        f"ssh {compatibility_options} root@{ip_addr} 'killall gdbserver; killall app; killall sh'; true", None)
    PhonyTargets(
        "scp", f"scp -O {compatibility_options} DS2021 root@{ip_addr}:/tmp/app", [target_prog, "kill-remote"])
    PhonyTargets(
        'ssh', f"ssh {compatibility_options} root@{ip_addr}", [])
    PhonyTargets(
        'run-remote',
        f"ssh {compatibility_options} root@{ip_addr} /tmp/app",
        'scp')
    PhonyTargets(
        "update-remote",
        f'ssh {compatibility_options} root@{ip_addr} "mount -o rw,remount / && cp /tmp/app /root/app && sync && reboot && exit"', "scp")
    PhonyTargets(
        "debug",
        f"ssh {compatibility_options} root@{ip_addr} gdbserver localhost:1235 /tmp/app", "scp")
    PhonyTargets(
        "run-remote",
        f"ssh {compatibility_options} root@{ip_addr} /tmp/app",
        "scp")

    Depends(simulated_prog, compileDB)
    Default(simulated_prog)
    Alias("target", target_prog)
    Alias("all", [target_prog, simulated_prog])


main()
