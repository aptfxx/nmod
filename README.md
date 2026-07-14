<img width="480" height="270" alt="nmod" src="https://github.com/user-attachments/assets/0630164d-2611-44e8-9f9b-0008ee6443e1" />

## nMOD ≽(•⩊ •マ≼
A no-bullshit minecraft mod loader.

Ever wanted to share Mincraft mods with your friends?
Have they ever been so tech illiterate that they can't locate the mods folder?
Have all the other mod loaders been full of bloat and online?

Well, welcome nMOD!
Written in C++, nMOD is a lightweight mod loader for minecraft. 
It doesn't save anything to the web, doesnt need to install anything on your PC. 
And you can just delete it after.

## Usage ˖˚⊹ ꣑ৎ‎

__**Local Install (Recommended)**__

nMOD expects one dependency: a folder titled 'nmods' in the working directory. If you put the file in Downloads, it expects Downloads\nmods.
This folder should include all of the mods you want others to install
Bundle that into a zip along with the nmod file, and send it to whoever the hell you want!

Use the command `createcontext` to create a file that describes your modpack even more. For example, the minecraft version and loader.
Bundle the context.nmod file with your .zip and .exe

__**Web Install**__

To install a modpack from the web, the format is `[source].modpack`. The source is from **github**  
The github repo must have a release, and the .modpack will search the **latest release** for a zip with that name.  
For example, `[https://github.com/aptfxx/nmod].hi` will fetch "hi.zip" from the latest repository in nmod  
It is very highly important that you makee sure what mods you're downloading! sha256sums are given to you to double-check with their official download sources.  

Note: the .zip only contains mods, context.nmod is not supported.
If you do not want web install at all, download [v1](https://github.com/aptfxx/nmod/releases/tag/v1)  

## What's Different

You could just write a one-line script to copy everything, right?
Yes, absolutely! Copying files isnt the only thing nMOD is good for. 

It has the capability to locally store backups (unless downloading from the web, web backup support coming in v2), and instantly restore them later.
One master script for managing your minecraft mods? Neat.

## Other Stuff
View source code [here](https://github.com/aptfxx/nmod/blob/main/nmod.cpp) *(Note: We do not provide archived source code. Only the latest update is shown in that file)*  
View sha256sums [here](https://github.com/aptfxx/nmod/blob/main/sha256sums.txt)


### ⋆｡ﾟ☁︎｡⋆｡ ﾟ☾ ﾟ｡⋆


### ⚖️ nMOD is under the GPL 3.0 ⚖️

```

             *     ,MMM8&&&.            *
                  MMMM88&&&&&    .
                 MMMM88&&&&&&&
     *           MMM88&&&&&&&&
                 MMM88&&&&&&&&
                 'MMM88&&&&&&'
                   'MMM8&&&'      *    
           /\/|_      __/\\
          /    -\    /-   ~\  .              '
          \    = Y =T_ =   /
           )==*(`     `) ~ \
          /     \     /     \
          |     |     ) ~   (
         /       \   /     ~ \
         \       /   \~     ~/
  jgs_/\_/\__  _/_/\_/\__~__/_/\_/\_/\_/\_/\_
  |  |  |  | ) ) |  |  | ((  |  |  |  |  |  |
  |  |  |  |( (  |  |  |  \\ |  |  |  |  |  |
  |  |  |  | )_) |  |  |  |))|  |  |  |  |  |
  |  |  |  |  |  |  |  |  (/ |  |  |  |  |  |
  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |

            APTITUDE | 2026
 ۫         𐂯ᩙ᩠𓏼    ׄ   ♡゙    ̹ ̜ㅤ

```
