fake_function() {
    // constexpr const char *accessDeniedStrings[SERVER_ACCESSDENIED_MAX]
    // We cannot run gettext inside a header file, so do this... >_<
    gettext("Invalid password");
    gettext("Your client sent something the server didn't expect.  Try reconnecting or updating your client.");
    gettext("The server is running in simple singleplayer mode.  You cannot connect.");
    gettext("Your client's version is not supported.\nPlease contact the server administrator.");
    gettext("Player name contains disallowed characters");
    gettext("Player name not allowed");
    gettext("Too many users");
    gettext("Empty passwords are disallowed.  Set a password and try again.");
    gettext("Another client is connected with this name.  If your client closed unexpectedly, try again in a minute.");
    gettext("Internal server error");
    gettext("Server shutting down");
    gettext("The server has experienced an internal error.  You will now be disconnected.");
}
