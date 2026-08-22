# Windows backend

`WindowsSystemInputBackend` is selected only under `_WIN32`; CMake excludes its
source on Linux and links `user32` on Windows. Core contracts and tests contain
no `Windows.h`.

Initialization reads the entire virtual desktop with
`SM_XVIRTUALSCREEN/YVIRTUALSCREEN/CXVIRTUALSCREEN/CYVIRTUALSCREEN` and rejects
non-positive dimensions. Movement calls the separately tested
`desktopPixelToSendInputAbsolute` helper and sends one `INPUT_MOUSE` with
MOVE, ABSOLUTE and VIRTUALDESK flags. Primary DOWN/UP send one transition input
with LEFTDOWN/LEFTUP.

Every call requires `SendInput(...) == 1`. Failure is stored as a structured
string containing the inserted count and numeric `GetLastError`, with an
explicit note that UIPI cannot be distinguished. No exception, elevation or
unbounded retry occurs.

The backend does not own logical button state; `ActionDispatcher` does. This
prevents platform code from guessing whether an UP belongs to c0ntrol.

The Windows SDK/toolchain was absent in the validation environment, so backend
compilation is NOT AVAILABLE and native integration is NOT TESTED. The normal
suite never calls real `SendInput`.
