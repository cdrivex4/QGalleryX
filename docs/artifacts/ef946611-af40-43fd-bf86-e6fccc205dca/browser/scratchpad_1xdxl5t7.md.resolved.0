# Bitsearch.to Analysis

## Search URL
`https://bitsearch.to/search?q={query}`

## HTML Structure for Search Results
Each result is contained in a card-like `div`.

### Key Selectors:
- **Result Row**: `div.w-full.bg-white.rounded-lg.shadow-sm`
- **Title**: `h5.text-lg.font-bold a` or `a.hover:text-primary`
- **Magnet Link**: `a[href^="magnet:"]`
- **Torrent Link**: `a[href^="/download/torrent/"]`
- **Size**: `div` containing download/folder icon and text like "GB" or "MB".
- **Seeders**: `div` containing up-arrow icon and text like "seeders".
- **Leechers**: `div` containing down-arrow icon and text like "leechers".

### Sample HTML Snippet (Simplified):
```html
<div class="search-result w-full bg-white rounded-lg shadow-sm p-4 ...">
    <h5 class="text-lg font-bold ...">
        <a class="hover:text-primary ..." href="/torrent/...">Senua Saga Hellblade II [DODI Repack]</a>
    </h5>
    <div class="flex items-center ...">
        <span>39.31 GB</span>
        <span>5/22/2024</span>
    </div>
    <div class="flex items-center ...">
        <span class="text-green-600">143 seeders</span>
        <span class="text-red-600">1343 leechers</span>
    </div>
    <div class="flex ...">
        <a class="bg-blue-600 ..." href="/download/torrent/...">Torrent</a>
        <a class="bg-green-600 ..." href="magnet:?xt=urn:btih:...">Magnet</a>
    </div>
</div>
```
