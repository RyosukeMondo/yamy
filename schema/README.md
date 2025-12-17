# JSON Schema for YAMY Configuration

This directory contains the JSON Schema definition for YAMY configuration files.

## What is JSON Schema?

JSON Schema is a vocabulary that allows you to annotate and validate JSON documents. It provides:

- **Validation**: Automatically check if your config file is correct
- **IDE Support**: Get autocomplete and inline documentation in your editor
- **Documentation**: Self-documenting format with descriptions

## Using the Schema

### In VSCode

Add this to the top of your YAMY config file:

```json
{
  "$schema": "../schema/config.schema.json",
  "version": "2.0",
  ...
}
```

VSCode will automatically:
- Show autocomplete suggestions for fields
- Display descriptions when you hover over properties
- Highlight validation errors in real-time

### In Other IDEs

Most modern IDEs (IntelliJ, Sublime Text, etc.) support JSON Schema. Consult your IDE's documentation for how to associate JSON files with schemas.

### Command-Line Validation

You can validate your config file using Python:

```bash
pip install jsonschema
python3 -c "
import json
from jsonschema import validate

with open('schema/config.schema.json') as f:
    schema = json.load(f)
with open('keymaps/my-config.json') as f:
    config = json.load(f)

validate(instance=config, schema=schema)
print('Config is valid!')
"
```

Or using Node.js with `ajv-cli`:

```bash
npm install -g ajv-cli
ajv validate -s schema/config.schema.json -d keymaps/my-config.json
```

## Schema Reference

See [docs/json-schema.md](../docs/json-schema.md) for detailed documentation of the configuration format.

## Schema Version

Current schema version: **draft-07**

This ensures compatibility with most tools and validators.
