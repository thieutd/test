namespace server::models::internal
{

// REFERENCE: https://medium.com/@nerudaj/tuesday-coding-tip-66-consteval-assertions-719098a77306
inline void you_see_this_error_because_you_try_to_clear_a_non_existent_field() noexcept
{
}
inline void you_see_this_error_because_you_try_to_replace_a_non_existent_field() noexcept
{
}

template <std::size_t N>
consteval std::array<const char *, N> ClearFields(const std::array<const char *, N> &fields,
                                                  const std::initializer_list<const char *> &empty_fields)
{
    auto new_fields{fields};
    for (auto &empty_field : empty_fields)
    {
        if (auto it = std::ranges::find(new_fields, empty_field); it != new_fields.end())
        {
            *it = "";
        }
        else
        {
            you_see_this_error_because_you_try_to_clear_a_non_existent_field();
        }
    }
    return new_fields;
}

template <std::size_t N>
consteval std::array<const char *, N> ReplaceFields(
    const std::array<const char *, N> &fields,
    const std::initializer_list<std::pair<const char *, const char *>> &replacements)
{
    auto new_fields{fields};
    for (auto &[old_field, new_field] : replacements)
    {
        if (auto it = std::ranges::find(new_fields, old_field); it != new_fields.end())
        {
            *it = new_field;
        }
        else
        {
            you_see_this_error_because_you_try_to_replace_a_non_existent_field();
        }
    }
    return new_fields;
}

// User table
constexpr std::array kUserFieldsArray = {"id",         "username",   "password",  "role",
                                         "avatar_url", "created_at", "deleted_at"};

constexpr std::array kUserRegistrationFieldsArray =
    ClearFields(kUserFieldsArray, {"id", "role", "created_at", "deleted_at"});
constexpr std::array kUserInfoFieldsArray = ClearFields(kUserFieldsArray, {"password", "deleted_at"});
constexpr std::array kUserRedisInfoFieldsArray =
    ClearFields(kUserFieldsArray, {"password", "avatar_url", "created_at", "deleted_at"});
constexpr std::array kUserCreationByAdminArray = ClearFields(kUserFieldsArray, {"id", "created_at", "deleted_at"});
constexpr std::array kUserUpdateFieldsArray = ClearFields(kUserFieldsArray, {"id", "role", "created_at", "deleted_at"});
constexpr std::array kUserUpdateByAdminFieldsArray = ClearFields(kUserFieldsArray, {"id", "created_at", "deleted_at"});

// Room table fields
constexpr std::array kRoomFieldsArray = {"id",         "name",       "type",       "description",
                                         "avatar_url", "created_at", "deleted_at", "last_message_id"};

constexpr std::array kRoomCreationFieldsArray = ClearFields(kRoomFieldsArray, {"id", "created_at", "deleted_at"});
constexpr std::array kRoomInfoFieldsArray = ClearFields(kRoomFieldsArray, {"deleted_at", "last_message_id"});

constexpr std::array kCommonRoomsViewFieldsArray = {"user1_id",    "user2_id",   "id",        "name",
                                                    "description", "avatar_url", "created_at"};
constexpr std::array kCommonRoomsViewResultFieldsArray =
    ClearFields(kCommonRoomsViewFieldsArray, {"user1_id", "user2_id"});

constexpr std::array kJoinedRoomsViewFieldsArray = {"user_id",    "id",         "name", "description",
                                                    "avatar_url", "created_at", "role", "joined_at"};
constexpr std::array kJoinedRoomsViewResultFieldsArray =
    ClearFields(kJoinedRoomsViewFieldsArray, {"user_id", "role", "joined_at"});
} // namespace server::models::internal
