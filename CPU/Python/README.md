# Source Code for The Other Metrics

## Descriptions

- `./offine`: source codes for offline queries evaluation.
- `./online`: source codes for online queries evaluation.

## Run

To run the codes with your own datasets, you should first comment out the codes of reading data, i.e., “import utils.File_Util” and the first three lines in each main function, as shown below.

```shell
# read file
df = utils.File_Util.read_single_file(49, True, unique_tag=False)
F = df['tag1']).
```
Then input each item of you dataset into the parameter ‘F’, which is actually a list in Python. 



