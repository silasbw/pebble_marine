
#
# This file is the default set of rules to compile a Pebble project.
#
# Feel free to customize this to your needs.
#

from sh import jshint
import os.path

top = '.'
out = 'build'
js_sources = [
    'src/js/co_ops.js',
    'src/js/gps.js',
    'src/js/conditions.js',
    'src/js/friscosailing.js',
    'src/js/main.js'
]
built_js = 'src/js/pebble-js-app.js'


def options(ctx):
    ctx.load('pebble_sdk')


def configure(ctx):
    ctx.load('pebble_sdk')
    jshint.bake(['--config', 'pebble-jshintrc'])


def concatenate_js(task):
    inputs = (input.abspath() for input in task.inputs)
    with open(task.outputs[0].abspath(), 'w') as outfile:
        for filename in inputs:
            with open(filename) as infile:
                for line in infile:
                    outfile.write(line)

def build(ctx):
    ctx.load('pebble_sdk')

    for filename in js_sources:
        jshint(filename)

    ctx(rule=concatenate_js, source=' '.join(js_sources), target=built_js)

    build_worker = os.path.exists('worker_src')
    binaries = []

    for p in ctx.env.TARGET_PLATFORMS:
        ctx.set_env(ctx.all_envs[p])
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf='{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_program(source=ctx.path.ant_glob('src/**/*.c'),
        target=app_elf)

        if build_worker:
            worker_elf='{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': p, 'app_elf': app_elf, 'worker_elf': worker_elf})
            ctx.pbl_worker(source=ctx.path.ant_glob('worker_src/**/*.c'),
            target=worker_elf)
        else:
            binaries.append({'platform': p, 'app_elf': app_elf})

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries,
                   js=ctx.path.ctx.path.find_or_declare('src/js/pebble-js-app.js'))
