# Contributing

Contributions are welcome! Here's how you can help:

- [Contributing code](#code)
- [Reporting issues](#issues)
- [Requesting features](#feature-requests)
- [Translating](#translations)
- [Donating](#donations)

## Code

If you are planning to start some significant coding, you would benefit from asking first on [our IRC channel](http://www.minetest.net/irc/) before starting.

1. [Fork](https://help.github.com/articles/fork-a-repo/) the repository and [clone](https://help.github.com/articles/cloning-a-repository/) your fork.

2. Start coding!
    - Refer to the [Lua API](https://github.com/minetest/minetest/blob/master/doc/lua_api.txt), [Developer Wiki](http://dev.minetest.net/Main_Page) and other [documentation](https://github.com/minetest/minetest/tree/master/doc).
    - Follow the [C/C++](http://dev.minetest.net/Code_style_guidelines) and [Lua](http://dev.minetest.net/Lua_code_style_guidelines) code style guidelines.
    - Check your code works as expected and document any changes to the Lua API.

3. Commit & [push](https://help.github.com/articles/pushing-to-a-remote/) your changes to a new branch (not `master`, one change per branch)
    - Commit messages should:
        - Use the present tense
        - Have a title which begins with a capital letter
        - Be descriptive. (e.g. no `Update init.lua` or `Fix a problem`)
        - Have a first line with less than *80 characters* and have a second line that is *empty*

4. Once you are happy with your changes, submit a pull request.
     - Open the [pull-request form](https://github.com/minetest/minetest/pull/new/master)
     - Add a short description explaining briefly what you've done (or if it's a work-in-progress - what you need to do)

##### A pull-request is considered merge-able when:

1. It follows the [roadmap](https://forum.minetest.net/viewtopic.php?t=9177) in some way and fits the whole picture of the project.
2. It works.
3. It follows the code style for [C/C++](http://dev.minetest.net/Code_style_guidelines) or [Lua](http://dev.minetest.net/Lua_code_style_guidelines).
4. The code's interfaces are well designed, regardless of other aspects that might need more work in the future.
5. It uses protocols and formats which include the required compatibility.

## Issues

If you experience an issue, we would like to know the details - especially when a stable release is on the way.

1. Do a quick search on GitHub to check if the issue has already been reported.
2. Is it an issue with the Minetest *engine*? If not, report it [elsewhere](http://www.minetest.net/development/#reporting-issues).
3. [Open an issue](https://github.com/minetest/minetest/issues/new) and describe the issue you are having - you could include:
     - Error logs (check the bottom of the `debug.txt` file)
     - Screenshots
     - Ways you have tried to solve the issue, and whether they worked or not
     - Your Minetest version and the content (subgames, mods or texture packs) you have installed
     - Your platform (e.g. Windows 10 or Ubuntu 15.04 x64)

After reporting you should aim to answer questions or clarifications as this helps pinpoint the cause of the issue (if you don't do this your issue may be closed after 1 month).

## Feature requests

Feature requests are welcome but take a moment to see if your idea follows the [roadmap](https://forum.minetest.net/viewtopic.php?t=9177) in some way and fits the whole picture of the project. You should provide a clear explanation with as much detail as possible.

## Translations

Translations of Minetest are performed using Weblate. You can access the project page  with a list of current languages [here](https://hosted.weblate.org/projects/minetest/minetest/).

### Donations

If you'd like to monetarily support Minetest development, you can find donation methods on [our website](http://www.minetest.net/development/#donate).

# Maintaining

*This is a concise version of the [Rules & Guidelines](http://dev.minetest.net/Category:Rules_and_Guidelines) on the developer wiki.*

These notes are for those who have push access Minetest (core developers / maintainers).

- See the [project organisation](http://dev.minetest.net/Organisation) for the people involved.

## Reviewing pull requests

Pull requests should be reviewed and, if appropriate, checked if they achieve their intended purpose. You can show that you are in the process of, or will review the pull request by commenting *"Looks good"* or something similar.

**If the pull-request is not [merge-able](#a-pull-request-is-considered-merge-able-when):**

Submit a comment explaining to the author what they need to change to make the pull-request merge-able.

- If the author comments or makes changes to the pull-request, it can be reviewed again.
- If no response is made from the author within 1 month (when improvements are suggested or a question is asked), it can be closed.

**If the pull-request is [merge-able](#a-pull-request-is-considered-merge-able-when):**

Submit a :+1: (+1) or "Looks good" comment to show you believe the pull-request should be merged. "Looks good" comments often signify that the patch might require (more) testing.

- Two core developers must agree to the merge before it is carried out and both should +1 the pull request.
- Who intends to merge the pull-request should follow the commit rules:
    - The title should follow the commit guidelines (title starts with a capital letter, present tense, descriptive).
    - Don't modify history older than 10 minutes.
    - Use rebase, not merge to get linear history:
    - `curl https://github.com/minetest/minetest/pull/1.patch | git am`

## Reviewing issues and feature requests

- If an issue does not get a response from its author within 1 month (when requiring more details), it can be closed.
- When an issue is a duplicate, refer to the first ones and close the later ones.
- Tag issues with the appropriate [labels](https://github.com/minetest/minetest/labels) for devices, platforms etc.

## Releasing a new version

*Refer to [dev.minetest.net/Releasing_Minetest](http://dev.minetest.net/Releasing_Minetest)*
