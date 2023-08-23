CREATE TABLE IF NOT EXISTS domain (
    domain_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(255) NOT NULL,
    display_name VARCHAR(255) NOT NULL
);

CREATE TABLE IF NOT EXISTS config (
    config_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    domain_id UUID NOT NULL,
    display_name VARCHAR(255) NOT NULL,

    light_config JSONB NOT NULL,

    FOREIGN KEY (domain_id) REFERENCES domain(domain_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS device (
    device_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    domain_id UUID NOT NULL,
    display_name VARCHAR(255) NOT NULL,

    mac VARCHAR(18) NOT NULL UNIQUE,
    config_id UUID NOT NULL,
    description TEXT,
    timezone VARCHAR(64) NOT NULL,

    FOREIGN KEY (domain_id) REFERENCES domain(domain_id) ON DELETE CASCADE,
    FOREIGN KEY (config_id) REFERENCES config(config_id)
);