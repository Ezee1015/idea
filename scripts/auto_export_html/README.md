# Auto export HTML

This script will detect when you try to open the `index.html` and regenerate the `generated.html`.

## Dependencies

- `inotify-tools`
- `base64`
- `imagemagick convert`

## How it works?

1. You have to run this script at startup: `idea_auto_export_html --daemon`
2. Now, whenever you open `index.html` with the browser the script will detect that and it's going to rebuild the `generated.html`
3. After one second in the `index.html`, the browser should redirect you to the `generated.html`

> There's a possible race condition between the script and the browser: if the
> script takes more than 1 second to generate the HTML (practically impossible
> in normal conditions), the browser is going to load an old or a non-existent
> `generated.html` file
