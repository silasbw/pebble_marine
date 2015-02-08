
#
# This file is the default set of rules to compile a Pebble project.
#
# Feel free to customize this to your needs.
#

from sh import jshint

top = '.'
out = 'build'

js_sources = [
    'src/js/co_ops.js',
    'src/js/gps.js',
    'src/js/conditions.js',

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

    ctx.pbl_program(source=ctx.path.ant_glob('src/**/*.c'),
                    target='pebble-app.elf')

    ctx.pbl_bundle(elf='pebble-app.elf',
                   js=ctx.path.ctx.path.find_or_declare('src/js/pebble-js-app.js'))
