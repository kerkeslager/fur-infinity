# fur-infinity
Hopefully the last attempt at creating the Fur programming language, using newly learned skills from reading Crafting Interpreters.

## Roadmap

* **0.1**
  - Garbage collection
  - Ropes
  - Closures
* **0.2**
  - Threading (10k threads on my MacBook or it isn't done)
  - Message passing queues
* **0.3**
  - Comprehensive spec
  - Comprehensive tests of that spec
  - Standard library:
    * JSON
    * Regular expressions
    * Parser generator
    * Commonmark
* **0.4**
  - Beginner tutorials
  - Philosophy docs
  - Performance tests
  - Standard library:
    - [GMP](https://gmplib.org/repo/)
* **0.5**
  - Standard library:
    * Security
    * Networking
    * Database interfaces
* **0.6**
  - Standard library:
    * Webserver
      - HTTPS
      - Rate limiting/blacklisting
      - Caching
      - Isn't that enough?

* **0.7** (Leave time for stuff to come up)
* **0.8** Core language stable
* **0.9** Standard library stable
* **1.0** Once supported always supported, so it better be good.

## Feature Ideas

### Imports

An import statement might look like:

```
import './foo.fur' as foo, # Relative-to-current-file import, supports ..
  'bar.fur' as bar, # Relative to project root
  'baz' as baz # Since baz doesn't start with . or end with .fur, it's a standard library

do_work() # This doesn't wait for imports to complete, executes immediately
bar.do_work() # This doesn't wait for foo or baz imports to complete, executes as soon as do_work() and bar import completes
```

1. If another thread is already importing a module, we don't spin up a thread for that import.
2. Module code is run exactly once.
3. Imports do not block the current thread, but references to the imported module do.
4. In the case where a module is imported and then immediately used, the import occurs on the current thread (if not already happening on another thread)
  since the current thread will be blocked anyway (we can detect this at compile time).
5. If do_work() is small, we may detect that spinning up a new thread to import bar costs more than running the import and do_work() in sequence. However,
  we can't guarantee detecting this at compile time as that would require solving the halting problem for do_work()

### Template
```
import 'template' as template

t = template.from_file('/my/cool/template.furt')
rendered_string = t(foo=1, bar=[1,2,3]) # Templates get compiled into functions like this
```
