INSERT INTO device (
    device_id,
    domain_id,
    display_name,
    mac,
    config_id,
    description,
    timezone,
    last_poll
) VALUES (
    '00000000-0000-0000-0000-000000000000'::UUID,
    '00000000-0000-0000-0000-000000000000'::UUID,
    'Test Display',
    '00:00:00:00:00:00',
    '00000000-0000-0000-0000-000000000000'::UUID,
    'This describes a device',
    'America/Montreal',
    NULL
)