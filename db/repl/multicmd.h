// @file multicmd.h

/**
*    Copyright (C) 2008 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "../../util/background.h"
#include "connections.h"

namespace mongo { 

    struct Target {
        Target(string hostport) : toHost(hostport), ok(false) { }
        Target() : ok(false) { }
        string toHost;
        bool ok;
        BSONObj result;
    };

    /* -- implementation ------------- */

    class _MultiCommandJob : public BackgroundJob { 
    public:
        BSONObj& cmd;
        Target& d;
        _MultiCommandJob(BSONObj& _cmd, Target& _d) : cmd(_cmd), d(_d) { }
    private:
        string name() { return "MultiCommandJob"; }
        void run() {
            try { 
                ScopedConn c(d.toHost);
                d.ok = c->runCommand("admin", cmd, d.result);
            }
            catch(DBException&) { 
                DEV log() << "dev caught dbexception on multiCommand " << d.toHost << rsLog;
            }
        }
    };

    inline void multiCommand(BSONObj cmd, list<Target>& L) { 
        typedef shared_ptr<_MultiCommandJob> P;
        list<P> jobs;
        list<BackgroundJob *> _jobs;

        for( list<Target>::iterator i = L.begin(); i != L.end(); i++ ) { 
            Target& d = *i;
            _MultiCommandJob *j = new _MultiCommandJob(cmd, d);
            jobs.push_back(P(j));
            _jobs.push_back(j);
        }

        BackgroundJob::go(_jobs);
        BackgroundJob::wait(_jobs,5);
    }

}
