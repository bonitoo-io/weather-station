#ifndef WS_MIGRATOR_H
#define WS_MIGRATOR_H

#include "FSPersistance.h"

class Migrator {
  private:
    String _error;
    FSPersistence *_pFsp;
  public:
    Migrator(FSPersistence *pFsp):_pFsp(pFsp) {};
    bool run();
    String getError() const { return _error; }
  protected:
    bool wifiMigration();
};

#endif //WS_MIGRATOR_H
