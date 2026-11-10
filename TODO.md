
TODOs:
- [ ] Goal Get rid of this file

- [ ] Minor database improvments
    - [ ] Name Index?

- [ ] Bulk Edit
    - [ ] Bulk Delete

- [ ] Stls improvements
    - [ ] Colorized output
    - [ ] Unicode
    - [ ] query tags
    - [ ] Limit + Offset

- [ ] Update Build tool to produce Static Lib
    - [ ] Tests + application should use static lib
- [ ] Add Conditional Build
    - [ ] Only rebuild tests if src is newer than dst

- [X] Update search to support searching by name
    - [X] fuzzy name search
- [X] Update SearchWithExpr to Handle Priority and Completion
    - [X] Parse NUM (make everything a float)
    - [X] Parse CExpr
    - [X] Update Query to Handle
- [X] Implement tests
    - [X] Fix Tests (allow overriding config)
    - [X] Extend tests to test purely internal functions
- [X] CLI tool for searching
- [X] Editable format
    - [X] Format
    - [X] Serialize (convert to database format)
- [X] priority
- [X] Completion
- [X] Add ANY identifier
    - [X] matches all tags
- [X] Simple Combination search
>    ie: get todos with simpleEngine AND bugfix 
>    or get todos with bugfix OR feature
>    or get todos with simpleEngine AND (bugfix OR feature)
