INSERT INTO config (
    config_id,
    domain_id,
    display_name,
    poll_period,
    light_config,
    sensor_configs
) VALUES (
    '00000000-0000-0000-0000-000000000000'::UUID,
    '00000000-0000-0000-0000-000000000000'::UUID,
    'Test Config',
    '1H'::INTERVAL,
    '{}'::JSONB,
    '[]'::JSONB
)