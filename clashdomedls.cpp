#include "clashdomedls.hpp"

void clashdomedls::createduel(string state, string type, string game)
{
    require_auth(_self);

    duels _dl(CONTRACTN, CONTRACTN.value);

    _dl.emplace(CONTRACTN, [&](auto& new_duel) {
        new_duel.id = _dl.available_primary_key();
        new_duel.state = state;
        new_duel.type = type;
        new_duel.game = game;
    });
}