# nMOD
A no-bullshit minecraft mod loader.

Ever wanted to share Mincraft mods with your friends?
Have they ever been so tech illiterate that they can't locate the mods folder?
Have all the other mod loaders been full of bloat and online?

Well, welcome nMOD!
Written in C++, nMOD is a lightweight mod loader for minecraft. 
It doesn't save anything to the web, doesnt need to install anything on your PC. 
And you can just delete it after.

## Usage

nMOD expects one dependency: a folder titled 'nmods' in the working directory. If you put the file in Downloads, it expects Downloads\nmods.
This folder should include all of the mods you want others to install
Bundle that into a zip along with the nmod file, and send it to whoever the hell you want!

Use the command `createcontext` to create a file that describes your modpack even more. For example, the minecraft version and loader.
Bundle the context.nmod file with your .zip and .exe

## What's Different

You could just write a one-line script to copy everything, right?
Yes, absolutely! Copying files isnt the only thing nMOD is good for. 

It has the capability to locally store backups, and instantly restore them later.
One master script for managing your minecraft mods? Neat.

## Other Stuff
View source code [here](https://github.com/aptfxx/nmod/blob/main/nmod.cpp)
View sha256sums [here](https://github.com/aptfxx/nmod/blob/main/sha256sums.txt)

