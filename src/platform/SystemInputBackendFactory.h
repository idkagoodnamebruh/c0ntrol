#ifndef SYSTEMINPUTBACKENDFACTORY_H
#define SYSTEMINPUTBACKENDFACTORY_H

#include <memory>

#include "src/core/actions/ISystemInputBackend.h"

std::unique_ptr<ISystemInputBackend> createSystemInputBackend();

#endif // SYSTEMINPUTBACKENDFACTORY_H
