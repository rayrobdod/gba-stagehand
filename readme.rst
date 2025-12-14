Stagehand
=============

You say "just use a pre-existing engine to make your game."
I say "Making the engine is the fun part."

Basically, doing my own thing while writing a GBA game.
Where "my one thing" includes things like describing hardware registers with real ``volatile struct`` variables
instead of macros casting integers to ``uint16_t *`` and building values with bitshift macros.

Aside from the gbafix tool,
the license of the files in this repository is CC0 1.0 Universal,
so you can treat all the files as if they were in the public domain.

Instructions
------------

You need the ``gcc-arm-none-eabi`` toolchain. If you're on Linux you can just
use your package manager. For example, in Ubuntu:

.. code:: bash

    sudo apt install gcc-arm-none-eabi

This will add the compiler to your ``PATH`` as well.

You can also download prebuilt binaries from `Arm's GNU toolchain downloads
website`_. In this case, you will need to manually add this toolchain to your
``PATH``.

Then, from the root folder, type:

.. code:: bash

    make

That's all!

.. _Arm's GNU toolchain downloads website: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads
