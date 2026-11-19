
# SimpleTODO

This is a simple library and toolset for managing
system wide task/issue tracking.

This was designed to make working on multiple projects
easier with a single central issue/todo system.


## Edit Todos

Here is the general format for a todo:
```md
# todo_name
Completion: 0%
Priority: 0.0
:tag1 :tag2

Body_data
```

Editing todos uses the `./build/stedit` command which will create
a temporary markdown file in the format above. Each field can be edited
and when your text editor is closed the todo database is updated.

`./build/stedit` will use the `$EDITOR` environment variable when
editing todos.

To edit many todos at once you can use `./build/stedit -b` which opens todos in
the following format.

```md
- [X] 10% (0.0) todo1 :tag1 :tag2
- [X] 10% (0.0) todo2 :tag1 :tag2
- [X] 10% (0.0) todo3 :tag1 :tag2
- [X] 10% (0.0) todo4 :tag1 :tag2
```

The left checkbox is used to delete todos, unchecking it will mark a todo to be deleted.
The next number is the completion as a percent, followed by the priority in parentheses.
Then the todo name followed by a space separated list of tags.

When editing, as long as the todo name remains the same the changes will be applied to the
todo. Editing todos does not affect the body of the todo.

## Searching Todos

`./build/stls` exposes SimpleTodo's built-in expression based search.

Here is the formal grammar for the search

```
EXPR := OEXPR
     |  OPEXR AND EXPR

OEXPR := CEXPR
      |  CEXPR OR OEXPR
      |  UNARY OR OEXPR

CEXPR := ID
      |  ID CMP CEXPR

CMP := '<' 
    |  '>'
    |  '='
    |  '!='
    |  '<='
    |  '>='

UNARY := NOT UNARY
       | TAG
       | ANY
       | '(' EXPR ')'

TAG := ':' String

ID := 'ANY'
    | 'PRIORITY'
    | 'COMPLETION'

```

And here are some examples

```
:tag1 AND test
```
This searches for todos with `:tag1` and names which contain
`test`.

```
PRIORITY > 1.0 OR COMPLETION > 80
```
This searches for todos with priority greater than `1.0` or
are more than 80% complete.

```
:tag1 AND (:tag2 OR :tag3)
```
This searches for todos with tag1 and either tag2 or tag3.

There are also several flags which can be used to alter how the output is displayed, and also
can be used to search which tags exist.


>       -c, --color: bool8              Colorize Output Based on Completion
>       -u, --unicode: bool8            Use Unicode in Output
>       -p, --pretty_print: bool8       Colorize Output and Use Unicode
>       -m, --minimal: bool8            Disable Colorization and Unicode
>       -com, --completion: bool8       Display Completion Information
>       -pr, --priority: bool8          Display Priority
>       -md, --markdown: bool8          Display Output as Markdown List, --color is ignored if this is set
>       -tag, --search_tag: bool8       Search Tags Instead of Todos, disables the ':tag' syntax
>       -l, --limit: int                Limit the number of items shown
>       -o, --offset: int               Ignore the first offset items. If this is set and limit is not, limit is set to 1000.

These can be found in the help page as well.
## Configuration

All simpletodo tools can be configured via a `.todo.conf` file.
Tools will start in the directory they are called and walk upwards
adding `.todo.conf` files as they are found.

This means you can have local configurations which override the default
values for things like database location, or adding required tags which
will be automatically applied to todos.
