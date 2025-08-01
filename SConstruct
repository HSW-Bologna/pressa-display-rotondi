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
SIMULATOR = "simulator"
FREERTOS = f"{SIMULATOR}/freertos-simulator"
CJSON = f"{SIMULATOR}/cJSON"
B64 = f"{SIMULATOR}/b64"
LIBS = "components"
MAIN = "main"
LVGL = f"managed_components/lvgl__lvgl"


CFLAGS = [
    "-Wall",
    "-Wextra",
    "-Wno-unused-function",
    "-g",
    "-O0",
    "-DprojCOVERAGE_TEST=0",
    "-DBUILD_CONFIG_SIMULATED_APP=1",
    # "-std=c2x",
]

CPPPATH = [
    f"#{MAIN}", f"#{MAIN}/config", f"#{LVGL}", f"#{LIBS}/log/src",
    f"#{SIMULATOR}", f"#{SIMULATOR}/port",
    f"#{LIBS}/liblightmodbus-esp/repo/include", f"#{CJSON}", f"#{B64}"
]


def get_target(env, name, suffix="", dependencies=[]):
    def rchop(s, suffix):
        if suffix and s.endswith(suffix):
            return s[:-len(suffix)]
        return s

    sources = [File(filename)
               for filename in Path(f"{MAIN}/adapters").rglob('*.c')]
    sources += [File(filename)
                for filename in Path(f"{MAIN}/config").rglob('*.c')]
    sources += [File(filename)
                for filename in Path(f"{MAIN}/controller").rglob('*.c')]
    sources += [File(filename)
                for filename in Path(f"{MAIN}/model").rglob('*.c')]
    sources += [File(filename)
                for filename in Path(f"{MAIN}/services").rglob('*.c')]
    sources += [File(filename)
                for filename in Path(f"{SIMULATOR}/port").rglob('*.c')]
    sources += [File(filename)
                for filename in Path(f"{LVGL}/src").rglob('*.c')]
    sources += [File(f"{SIMULATOR}/simulator.c")]
    sources += [File(f'{CJSON}/cJSON.c')]
    sources += [File(f'{B64}/encode.c'),
                File(f'{B64}/decode.c'), File(f'{B64}/buffer.c')]

    pman_env = env
    (lv_pman, include) = SConscript(
        f'{LIBS}/c-page-manager/SConscript', variant_dir=f"build-{name}/lv_pman", exports=['pman_env'])
    env['CPPPATH'] += [include]

    c_watcher_env = env
    (watcher, include) = SConscript(f'{LIBS}/c-watcher/SConscript',
                                    variant_dir=f"build-{name}/c-watcher", exports=['c_watcher_env'])
    env['CPPPATH'] += [include]

    freertos_suffix = ""
    freertos_env = env
    (freertos, include) = SConscript(
        f'{FREERTOS}/SConscript', exports=['freertos_env', "freertos_suffix"])
    env['CPPPATH'] += [include]

    objects = [env.Object(
        f"{rchop(x.get_abspath(), '.c')}{suffix}", x) for x in sources]

    target = env.Program(name, objects + [lv_pman, watcher, freertos])
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

    env = Environment(**env_options)
    env.Tool('compilation_db')

    simulated_prog = get_target(
        env, SIMULATED_PROGRAM, dependencies=[])

    PhonyTargets('run', f"./{SIMULATED_PROGRAM}",
                 simulated_prog, env)
    compileDB = env.CompilationDatabase('compile_commands.json')

    Depends(simulated_prog, compileDB)
    Default(simulated_prog)


main()
