settings_test = executable(
    'test-drive-settings',
    'test-drive-settings.c',
    '../src/brasero-drive-settings.c',
    include_directories: [root_inc, include_directories('../src')],
    dependencies: [libbrasero_burn, gtk3],
    export_dynamic: true,
)

test(
    'drive-settings-detach',
    settings_test,
    env: {
        'GSETTINGS_BACKEND': 'memory',
        'GSETTINGS_SCHEMA_DIR': meson.project_build_root() / 'data',
    },
)
