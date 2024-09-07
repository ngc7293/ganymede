CREATE EXTENSION IF NOT EXISTS timescaledb;

CREATE TABLE "atmosphere" (
    observed_on       TIMESTAMP WITH TIME ZONE DEFAULT (NOW() AT TIME ZONE 'UTC') NOT NULL,
    domain_id         UUID                                                        NOT NULL,
    device_id         UUID                                                        NOT NULL,
    temperature       FLOAT,
    relative_humidity FLOAT,

    FOREIGN KEY (domain_id) REFERENCES domain(domain_id) ON DELETE CASCADE,
    FOREIGN KEY (device_id) REFERENCES device(device_id) ON DELETE CASCADE
);
SELECT create_hypertable('atmosphere', by_range('observed_on'));

CREATE TABLE "soil" (
    observed_on    TIMESTAMP WITH TIME ZONE DEFAULT (NOW() AT TIME ZONE 'UTC') NOT NULL,
    domain_id      UUID                                                        NOT NULL,
    device_id      UUID                                                        NOT NULL,
    temperature    FLOAT,
    humidity       FLOAT,

    FOREIGN KEY (domain_id) REFERENCES domain(domain_id) ON DELETE CASCADE,
    FOREIGN KEY (device_id) REFERENCES device(device_id) ON DELETE CASCADE
);
SELECT create_hypertable('soil', by_range('observed_on'));