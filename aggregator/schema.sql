DROP TABLE IF EXISTS data;
DROP TABLE IF EXISTS channels;

CREATE TABLE channels (
    ch_id INTEGER AUTO_INCREMENT NOT NULL,
    name TEXT NOT NULL,
    PRIMARY KEY pk_ch_id (ch_id)
);

CREATE TABLE data (
    ch_id INTEGER,
    timestamp BIGINT NOT NULL,
    voltage FLOAT NOT NULL,
    current FLOAT NOT NULL,
    power FLOAT NOT NULL,
    PRIMARY KEY pk_ch_id_timestamp (ch_id, timestamp),
    FOREIGN KEY fk_channels_ch_id (ch_id) REFERENCES channels (ch_id) ON DELETE CASCADE
);

INSERT INTO channels (name) VALUES ("kitchen");
INSERT INTO channels (name) VALUES ("house");

