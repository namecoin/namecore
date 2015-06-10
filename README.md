Namecoin Core integration/staging tree
=====================================

https://namecoin.info/

What is Namecoin? 
----------------

Namecoin is a decentralized open source information registration and transfer system based on the Bitcoin cryptocurrency.

What does it do?
----------------

Securely record and transfer arbitrary names (keys).
Attach a value (data) to the names
(up to 520 bytes, more in the future).
Transact namecoins, the digital currency (ℕ, NMC).
Namecoin was the first fork of Bitcoin and still is one of the most innovative altcoins. It was first to implement merged mining and a decentralized DNS. Namecoin squares Zooko's Triangle!

What can it be used for?
----------------

Protect free-speech rights online by making the web more resistant to censorship.
Access websites using the .bit domain (with TLS/SSL).
Store identity information such as email, GPG key, BTC address, TLS fingerprints, Bitmessage address, etc.
Human readable Tor .onion names/domains.
File signatures, Voting, bonds/stocks,/shares, web of trust, escrow and notary services (to be implemented).


License
-------

Namecoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see http://opensource.org/licenses/MIT.

Development process
-------------------

Developers work in their own trees, then submit pull requests when they think
their feature or bug fix is ready.

If it is a simple/trivial/non-controversial change, then one of the Bitcoin
development team members simply pulls it.

If it is a *more complicated or potentially controversial* change, then the patch
submitter will be asked to start a discussion (if they haven't already) on the
[forum](https://forum.namecoin.info/viewforum.php?f=8).

The patch will be accepted if there is broad consensus that it is a good thing.
Developers should expect to rework and resubmit patches if the code doesn't
match the project's coding conventions (see [doc/developer-notes.md](doc/developer-notes.md)) or are
controversial.

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/bitcoin/bitcoin/tags) are created
regularly to indicate new official, stable release versions of Bitcoin.

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write unit tests for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run (assuming they weren't disabled in configure) with: `make check`

### Manual Quality Assurance (QA) Testing

Large changes should have a test plan, and should be tested by somebody other
than the developer who wrote the code.
See https://github.com/bitcoin/QA/ for how to create a test plan.

Translations
------------

Changes to translations as well as new translations can be submitted to
[Bitcoin Core's Transifex page](https://www.transifex.com/projects/p/bitcoin/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.

Translators should also subscribe to the [mailing list](https://groups.google.com/forum/#!forum/bitcoin-translators).
