-- Enum Types
CREATE TYPE "user_role" AS ENUM ('admin', 'user');
CREATE TYPE "room_type" AS ENUM ('direct', 'group');
CREATE TYPE "room_role" AS ENUM ('admin', 'member');
CREATE TYPE "media_type" AS ENUM ('image', 'video', 'audio', 'document');

-- User Table
CREATE TABLE IF NOT EXISTS "user"
(
    id SERIAL PRIMARY KEY,
    username VARCHAR(255) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    role user_role NOT NULL DEFAULT 'user',
    avatar_url VARCHAR(1024),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP
);

-- Room Table
CREATE TABLE IF NOT EXISTS "room"
(
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
--    type room_type NOT NULL,
    type room_type NOT NULL DEFAULT 'group',
    description TEXT DEFAULT '',
    avatar_url VARCHAR(1024),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP,
    last_message_id INT,
    CONSTRAINT room_name_length CHECK (LENGTH(name) >= 3 AND LENGTH(name) <= 255)
);

-- Room Membership Table
CREATE TABLE IF NOT EXISTS "room_membership"
(
    user_id INT NOT NULL REFERENCES "user"(id),
    room_id INT NOT NULL REFERENCES "room"(id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP,
    role room_role NOT NULL DEFAULT 'member',
    PRIMARY KEY (user_id, room_id)
);

-- -- Direct Chat Table
-- CREATE TABLE IF NOT EXISTS "direct_chat"
-- (
--     id INT PRIMARY KEY REFERENCES "room"(id),
--     user1_id INT NOT NULL REFERENCES "user"(id),
--     user2_id INT NOT NULL REFERENCES "user"(id),
--     created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
--     UNIQUE (user1_id, user2_id)
-- );

-- Message Table
CREATE TABLE IF NOT EXISTS "message"
(
    id SERIAL PRIMARY KEY,
    user_id INT NOT NULL REFERENCES "user"(id),
    room_id INT NOT NULL REFERENCES "room"(id),
    content TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP
);

-- Add foreign key constraint for last_message_id in room table
ALTER TABLE "room"
ADD CONSTRAINT fk_room_last_message
FOREIGN KEY (last_message_id) REFERENCES "message"(id);

-- -- Media Attachment Table
-- CREATE TABLE IF NOT EXISTS "media_attachment"
-- (
--     id SERIAL PRIMARY KEY,
--     message_id INT NOT NULL REFERENCES "message"(id),
--     media_type media_type NOT NULL,
--     file_name VARCHAR(255) NOT NULL,
--     file_size BIGINT NOT NULL,
--     url VARCHAR(1024) NOT NULL,
--     mime_type VARCHAR(127) NOT NULL,
--     created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
-- );

-- -- Message Read Receipt Table
-- CREATE TABLE IF NOT EXISTS "message_read_receipt"
-- (
--     user_id INT NOT NULL REFERENCES "user"(id),
--     message_id INT NOT NULL REFERENCES "message"(id),
--     read_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
--     PRIMARY KEY (user_id, message_id)
-- );

-- -- Message Reaction Table
-- CREATE TABLE IF NOT EXISTS "message_reaction"
-- (
--     id SERIAL PRIMARY KEY,
--     message_id INT NOT NULL REFERENCES "message"(id),
--     user_id INT NOT NULL REFERENCES "user"(id),
--     emoji VARCHAR(10) NOT NULL,
--     created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
--     UNIQUE (message_id, user_id, emoji)
-- );

-- -- Pinned Message Table
-- CREATE TABLE IF NOT EXISTS "pinned_message"
-- (
--     id SERIAL PRIMARY KEY,
--     room_id INT NOT NULL REFERENCES "room"(id),
--     message_id INT NOT NULL REFERENCES "message"(id),
--     user_id INT NOT NULL REFERENCES "user"(id),
--     pinned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
-- );

-- -- Notification Table
-- CREATE TABLE IF NOT EXISTS "notification" (
--     id SERIAL PRIMARY KEY,
--     user_id INT NOT NULL REFERENCES "user"(id),
--     event_type VARCHAR(50) NOT NULL,
--     event_data JSONB NOT NULL,
--     is_read BOOLEAN DEFAULT FALSE,
--     created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
-- );

-- Trigger and function to update last_message_id in room table
CREATE FUNCTION update_room_last_message_id()
RETURNS TRIGGER AS $$
BEGIN
    UPDATE room
    SET last_message_id = NEW.id
    WHERE id = NEW.room_id;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER update_room_last_message_id
AFTER INSERT ON message
FOR EACH ROW
EXECUTE FUNCTION update_room_last_message_id();

-- Indexes
CREATE INDEX idx_user_username ON "user"(username);
CREATE INDEX idx_room_name ON "room"(name);
CREATE INDEX idx_message_room ON "message"(room_id, created_at DESC);
CREATE INDEX idx_message_user ON "message"(user_id);
CREATE INDEX idx_room_membership_user ON "room_membership"(user_id);
CREATE INDEX idx_message_created_at ON "message"(created_at);
CREATE INDEX idx_user_active ON "user"(id) 
    WHERE deleted_at IS NULL;
CREATE INDEX idx_room_active ON "room"(id) 
    WHERE deleted_at IS NULL;
CREATE INDEX idx_room_membership_active ON "room_membership"(room_id, user_id) 
    WHERE deleted_at IS NULL;
CREATE INDEX idx_room_last_message_id ON room (last_message_id);
CREATE INDEX idx_message_room_created_at ON message (room_id, created_at DESC);

-- Views
-- Get common rooms between two users
CREATE VIEW common_rooms_view AS
SELECT
    LEAST(rm1.user_id, rm2.user_id) AS user1_id,
    GREATEST(rm1.user_id, rm2.user_id) AS user2_id,
    r.id, r.name, r.description, r.avatar_url, r.created_at
FROM
    "room_membership" rm1
JOIN
    "room_membership" rm2 ON rm1.room_id = rm2.room_id
JOIN
    "room" r ON rm1.room_id = r.id
WHERE
    rm1.user_id <> rm2.user_id AND
    r.deleted_at IS NULL
ORDER BY
    r.created_at DESC;

-- Get a user's joined rooms
CREATE VIEW joined_rooms_view AS
SELECT
    rm.user_id,
    r.id, r.name, r.description, r.avatar_url, r.created_at,
    rm.role, rm.created_at AS joined_at
FROM
    "room_membership" rm
JOIN
    "room" r ON rm.room_id = r.id
WHERE
    r.deleted_at IS NULL AND rm.deleted_at IS NULL
ORDER BY
    r.created_at DESC;

-- Get user's rooms with latest messages
CREATE VIEW user_rooms_with_messages_view AS
SELECT 
    rm.user_id,
    r.id, r.name, r.type, r.avatar_url,
    m.id AS message_id,
    m.content AS message_content,
    m.created_at AS message_created_at,
    sender.id AS sender_id,
    sender.username AS sender_username,
    sender.avatar_url AS sender_avatar,
    rm.role AS user_room_role
FROM room r
INNER JOIN room_membership rm ON r.id = rm.room_id
LEFT JOIN message m ON m.id = r.last_message_id
LEFT JOIN "user" sender ON m.user_id = sender.id
WHERE 
    r.deleted_at IS NULL
    AND rm.deleted_at IS NULL
ORDER BY
    r.created_at DESC;
