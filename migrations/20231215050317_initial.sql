CREATE TYPE feature_type AS ENUM ('light');
CREATE TYPE component_type AS ENUM ('gpio_light');


CREATE TABLE IF NOT EXISTS domains (
    id           UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    display_name VARCHAR(255) NOT NULL
);

CREATE TABLE IF NOT EXISTS features (
    id           UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    domain_id    UUID         NOT NULL,
    display_name VARCHAR(255) NOT NULL,
    feature_type feature_type NOT NULL,

    FOREIGN KEY (domain_id) REFERENCES domains(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS profiles (
    id           UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    domain_id    UUID         NOT NULL,
    display_name VARCHAR(255) NOT NULL,

    FOREIGN KEY (domain_id) REFERENCES domains(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS feature_profiles (
    id           UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    domain_id    UUID         NOT NULL,
    display_name VARCHAR(255) NOT NULL,
    profile_id   UUID         NOT NULL,
    feature_id   UUID         NOT NULL,
    config       JSONB        NOT NULL,

    FOREIGN KEY (domain_id)  REFERENCES domains(id)  ON DELETE CASCADE,
    FOREIGN KEY (profile_id) REFERENCES profiles(id) ON DELETE CASCADE,
    FOREIGN KEY (feature_id) REFERENCES features(id) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS device_types (
    id           UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    domain_id    UUID         NOT NULL,
    display_name VARCHAR(255) NOT NULL,
    profile_id   UUID         NOT NULL,

    FOREIGN KEY (domain_id)  REFERENCES domains(id)  ON DELETE CASCADE,
    FOREIGN KEY (profile_id) REFERENCES profiles(id) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS device_features (
    id             UUID             PRIMARY KEY DEFAULT gen_random_uuid(),
    domain_id      UUID             NOT NULL,
    display_name   VARCHAR(255)     NOT NULL,
    device_type_id UUID           NOT NULL,
    feature_id     UUID           NOT NULL,
    component_type component_type NOT NULL,
    config         JSONB          NOT NULL,

    FOREIGN KEY (domain_id)      REFERENCES domains(id)      ON DELETE CASCADE,
    FOREIGN KEY (device_type_id) REFERENCES device_types(id) ON DELETE CASCADE,
    FOREIGN KEY (feature_id)     REFERENCES features(id)     ON DELETE RESTRICT
);


CREATE TABLE IF NOT EXISTS devices (
    id             UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    domain_id      UUID         NOT NULL,
    display_name   VARCHAR(255) NOT NULL,
    mac            VARCHAR(18)  NOT NULL UNIQUE,
    device_type_id UUID         NOT NULL,
    description    TEXT,
    timezone       VARCHAR(64)  NOT NULL,

    FOREIGN KEY (domain_id)      REFERENCES domains(id)      ON DELETE CASCADE,
    FOREIGN KEY (device_type_id) REFERENCES device_types(id) ON DELETE RESTRICT
);
