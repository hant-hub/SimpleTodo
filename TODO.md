
TODOs:
- [ ] Goal Get rid of this file

- [ ] Bulk Edit
    - [ ] Bulk Delete

- [ ] Stls improvements
    - [ ] Colorized output
    - [ ] Unicode
    - [ ] query tags
    - [ ] Limit + Offset

- [ ] Add Conditional Build
    - [ ] Only rebuild tests if src is newer than dst


- [X] Config Improvement
    - [X] Arbitary fields
    - [X] Multimap style values 
    - [X] 'overlay' logic
    - [X] core logic integration
- [X] Update Build tool to produce Static Lib
    - [X] Tests + application should use static lib
- [X] Minor database improvments
    - [X] Name Index?
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
