# Conway's Game of Life

Game accepts input on stdin of the form `(x, y)` one per line followed by `EOF` prior to beginning the simulation.
Alternatively, a filename may be specified on the command line in the [Run Length Encoded](http://www.conwaylife.com/w/index.php?title=Run_Length_Encoded) format.

## Controls

- `w`,`a`,`s`,`d`: Move viewport
- `-`,`+`:  Zoom viewport
- `{`, `}`: Change generation step delay
- `p`: Pause simulation
- `n`: Step to the next generation (e.g. during pause)
- `l`: Print all the currently live points to the console

## Notes

- The board exists in the int64 space and wraps on the edges to form a toroidal surface.
- Pattern Files in the `rle` directory are sourced from [LifeWiki](http://conwaylife.com/wiki/Main_Page) or generated using [tlrobinson/life-gen](https://github.com/tlrobinson/life-gen).