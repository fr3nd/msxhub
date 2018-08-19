# MsxHub

https://www.msxhub.com

[MsxHub](https://www.msxhub.com) is a MSX application to download and install software from the internet using an UNAPI compatible network card.

## Installation

### Requirements

* An MSX2 compatible computer
* MSX-DOS2
* [InterNestor Lite](https://www.konamiman.com/msx/msx-e.html#inl2)
* An UNAPI compatible network card

As far as I know, there is no emulator capable to emulate a network card on the MSX, so this software can only run on real hardware.

To install it in your MSX computer, make sure it's connected to the internet and run those commands from MSX-DOS2:
```
md A:\HUB
cd A:\HUB
hget http://msxhub.com/hub.com
```

### Download

* [Current releases](https://github.com/fr3nd/msxhub/releases)

## Usage

Before start using it, MsxHub needs to be configured:
```
hub configure
```

Install your first program ([msx-vi](https://github.com/fr3nd/msx-vi)):
```
hub install vi
```

List all packages available to download and install:
```
hub list
```

Show all installed packages:
```
hub installed
```

Uninstall a package:
```
hub uninstall vi
```

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

### Build requirements

* GNU Make
* Docker (It's using a docker image to compile using SDCC)

## License

[GNU General Public License v3.0](https://choosealicense.com/licenses/gpl-3.0/)

## Acknowledgements

* Nestor Soriano for creating InterNestor Lite and for releasing the source code of [hget](https://github.com/Konamiman/MSX/blob/master/SRC/NETWORK/hget.c).
* Javi Lavandeira for the fantastic [Relearning MSX](https://www.lavandeira.net/relearning-msx/) tutorials, that made me re-introduce to the MSX world.
* All the people behind the [MSX Assembly Page](http://map.grauw.nl/). Probably the biggest source of documentation for MSX developers.
* Everyone in the msx.org forums. You guys always helped me when I had questions.
