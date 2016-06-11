
#include "../../client/source/Lobby.h"

using namespace multislider;

class Callback
    : public HostCallback
{ };

int main()
{
    Lobby lobby;
    Callback callback;
    Host* host = lobby.becomeHost(&callback);
    return 0;
}
