
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

