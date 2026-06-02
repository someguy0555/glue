# Syntax

## Basics
### Statements
Unlike some other languages, LANGUAGE_NAME does not require the use of semicolons to denote the end of a statement - instead newlines act as such.
However the option is always there - if you want to have multiple statements in a single line, you can seperate them using semicolons.
```
let a
let b : Bool
let x : Int = 5; let y = c
```

### If statements and loops
If statements and loops are not expressions - they can't be used as such.
```
if <cond> <block>
if <cond>
    <stmt>
if <cond>; <stmt>
```
```
while <cond> <block>
while <cond>
    <stmt>
while <cond>; <stmt>

# Is the same as `while true`
loop <block>
```

```
for <variable> in <expression> <block>

for i in [1..100)
    print(i)
```

### Pattern matching
```
```

## Type system
LANGUAGE_NAME is statically, strongly typed, however it uses Hindley-Milner type inference,
allowing the user to eschew most types signatures.
It also supports algebraic effects.

## Default types
* Nil
* Bool
* Char
* Nat
* Int
* Real
* List(t)
* Tables {...}

### Creating new types
```
type Maybe(a) = Nothing | Just(a)
alias Mint = Maybe(int)
alias M = Maybe



```

### Effects
```
effect Yield(a) ctl yield(a) : a

with handler
    ctl yield(x) x * x
    [1,2,3]:traverse()

| Yield(a) => fn yield(x); 

handle [1,2,3]:traverse()
| ctl yield(a); a

handle ask-non-negative()
| ctl ask(a); a
| ctl raise(msg); print(msg)
| return(x) print(x)
```

## BNF

# Examples
```
# Types

effect Exception
| ctl throw(String)

type Tile
| Bomb
| NoBomb(Int)

type GameState
| STARTING
| RUNNING
| ENDED

type FlagOption
| FLAG
| REVEAL
| MASS_REVEAL

# Helpers

fn digit?(c)
    return 48 <= to_int(c) and to_int(c) <= 57

fn to_digit(c)
    if digit?(c); return to_int(c) - 48
    else throw("Cannot convert a non-digit to a number)

fn str_to_int(str)
{
    let num = 0
    let sign = 1

    str = str:filter(fn(c) return c /= 9 and c /= 32)

    if not str:all(fn(c) digit?(c) or c == '-')
        throw("String is not an integer")

    if str:count(fn(c) return c == '-') > 1
        throw("Integer contains multiple '-' characters")

    if str:head() == '-'
    {
        str = str:tail()
        sign = -1
    }

    for c in str
        num = num * 10 + c

    return sign * num
}

fn nat?(i)
    return i >= 0

fn get_nat()
    return get_line():str_to_int():check(nat?)

# Grid functions

fn modify_tile(grid, f, x, y)
{
    grid[x][y] = grid[x][y]:f()
    return grid
}

# fn generate_grid(x, y, bombs)
# {
#     let grid = NoBombs(0):repeat():repeat()
# }
# 
# fn toggle_flag(x, y)
# {
# }
# 
# fn reveal(x, y)     
# {
# }
# 
# fn mass_reveal(x, y)
# {
# }


fn minesweeper()
{
    let game
    loop
    {
        case game.state
        | STARTING =>
            {
                handle game.x = get_nat()
                | ctl throw(msg) { perror(msg); redo }
                end
                handle game.y = get_nat()
                | ctl throw(msg) { perror(msg); redo }
                end
                handle game.bombs = get_nat():check(fn(i) return i <= game.x * game.y)
                | ctl throw(msg) { perror(msg); redo }
                end
                
                game.grid = generate_grid(game.x, game.y, game.bombs)
            }
        | RUNNING =>
            {
                print(game.grid)

                let choice
                
                handle choice.x = get_nat():check(fn(i) return i <= game.x) | _ => redo end
                handle choice.y = get_nat():check(fn(i) return i <= game.y) | _ => redo end
                handle
                    choice.option = get_line()
                        :filter(fn(c) return whitespace?)
                        :check(fn(s) return s:length == 1)
                        :fn(c)
                        {
                            case c
                            | 'F' => return FLAG
                            | 'M' => return MASS_REVEAL
                            | 'R' => return REVEAL
                            | _   => throw("This flag option does not exist")
                            end
                        }
                | ctl throw(msg) { perror(msg); redo }
                end

                handle
                    case choice.option
                    | FLAG        => game.grid = game.grid:toggle_flag(game.x, game.y)
                    | REVEAL      => game.grid = game.grid:reveal(game.x, game.y)     
                    | MASS_REVEAL => game.grid = game.grid:mass_reveal(game.x, game.y)
                    end
                | ctl throw(msg) { perror(msg); redo }
                | ctl bomb_found()
                    {
                        print(game.grid)
                        print("Game over, start again?)

                        handle choice.option = get_line()
                            :filter(fn(c) return whitespace?)
                            :check(fn(s) return s:length == 1)
                            :uppercase
                            :fn(c)
                            {
                                case c
                                | 'Y' => game.state = STARTING
                                | 'N' => game.state = ENDED
                                | _   => throw("This option does not exist")
                            }
                        | ctl throw(msg) { perror(msg); redo }
                    }
                end
            }
        | ENDED => break
        end
    }
}

```

```
# Types

effect Exception
| ctl throw(String)

type Tile
| Bomb
| NoBomb(Int)

type GameState
| STARTING
| RUNNING
| ENDED

type FlagOption
| FLAG
| REVEAL
| MASS_REVEAL

# Helpers

fn digit?(c)
    return 48 <= to_int(c) and to_int(c) <= 57

fn to_digit(c)
    if digit?(c); return to_int(c) - 48
    else throw("Cannot convert a non-digit to a number)

fn str_to_int(str)
do
    let num = 0
    let sign = 1

    str = str:filter(fn(c) return c /= 9 and c /= 32)

    if not str:all(fn(c) digit?(c) or c == '-')
        throw("String is not an integer")

    if str:count(fn(c) return c == '-') > 1
        throw("Integer contains multiple '-' characters")

    if str:head() == '-'
    do
        str = str:tail()
        sign = -1
    end

    for c in str
        num = num * 10 + c

    return sign * num
end

fn nat?(i)
    return i >= 0

fn get_nat()
    return get_line():str_to_int():check(nat?)

# Grid functions

fn modify_tile(grid, f, x, y)
do
    grid[x][y] = grid[x][y]:f()
    return grid
end

# fn generate_grid(x, y, bombs)
# do
#     let grid = NoBombs(0):repeat():repeat()
# end
# 
# fn toggle_flag(x, y)
# do
# end
# 
# fn reveal(x, y)     
# do
# end
# 
# fn mass_reveal(x, y)
# do
# end


fn minesweeper()
do
    let game
    loop
    do
        case game.state
        | STARTING =>
            do
                handle game.x = get_nat()
                | ctl throw(msg) do perror(msg); redo end
                end
                handle game.y = get_nat()
                | ctl throw(msg) do perror(msg); redo end
                end
                handle game.bombs = get_nat():check(fn(i) return i <= game.x * game.y)
                | ctl throw(msg) do perror(msg); redo end
                end
                
                game.grid = generate_grid(game.x, game.y, game.bombs)
            end
        | RUNNING =>
            do
                print(game.grid)

                let choice
                
                handle choice.x = get_nat():check(fn(i) return i <= game.x) | _ => redo end
                handle choice.y = get_nat():check(fn(i) return i <= game.y) | _ => redo end
                handle
                    choice.option = get_line()
                        :filter(fn(c) return whitespace?)
                        :check(fn(s) return s:length == 1)
                        :fn(c)
                        do
                            case c
                            | 'F' => return FLAG
                            | 'M' => return MASS_REVEAL
                            | 'R' => return REVEAL
                            | _   => throw("This flag option does not exist")
                            end
                        end
                | ctl throw(msg) do perror(msg); redo end
                end

                handle
                    case choice.option
                    | FLAG        => game.grid = game.grid:toggle_flag(game.x, game.y)
                    | REVEAL      => game.grid = game.grid:reveal(game.x, game.y)     
                    | MASS_REVEAL => game.grid = game.grid:mass_reveal(game.x, game.y)
                    end
                | ctl throw(msg) do perror(msg); redo end
                | ctl bomb_found()
                    do
                        print(game.grid)
                        print("Game over, start again?)

                        handle choice.option = get_line()
                            :filter(fn(c) return whitespace?)
                            :check(fn(s) return s:length == 1)
                            :uppercase
                            :fn(c)
                            do
                                case c
                                | 'Y' => game.state = STARTING
                                | 'N' => game.state = ENDED
                                | _   => throw("This option does not exist")
                            end
                        | ctl throw(msg) do perror(msg); redo end
                    end
                end
            end
        | ENDED => break
        end
    end
end
```
