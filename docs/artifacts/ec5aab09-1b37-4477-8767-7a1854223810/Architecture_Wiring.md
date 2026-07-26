# Second Brain Architecture & Wiring Diagram

This diagram maps out exactly how data flows from your projects, through the Graphify extractor, up to the Hermes Gateway routing layer, and finally into the Roo-code modes.

```mermaid
flowchart TD
    User["User / Developer"] -->|Issues Prompts| Roo["Roo-code VS Code"]
    
    subgraph SecondBrain ["Second Brain System"]
        Roo -->|Invokes Configured Modes| Comparator["Project Comparator"]
        Roo -->|Invokes Configured Modes| Architect["Architect Mode"]
        
        Comparator -->|Queries| SQLite[("Local SQLite graph.db")]
        Architect -->|Queries| SQLite
        
        Indexer["bootstrap.py / Indexer"] -->|Loads extracted nodes| SQLite
    end

    subgraph GraphifyLayer ["Graphify Extraction Layer"]
        Indexer -->|Triggers CLI| Graphify["Graphify CLI"]
        Graphify -->|Reads and Parses| SourceCode[("Target Projects")]
        Graphify -.->|Outputs Semantic Data| GraphJSON["graph.json"]
        GraphJSON -.->|Ingested by| Indexer
    end

    subgraph RoutingLayer ["Routing Layer"]
        Graphify -->|Semantic LLM Requests| GatewayRouter["Hermes Gateway (Port 8666)"]
    end

    subgraph ExecutionLayer ["Execution Layer"]
        GatewayRouter ==>|Primary Route| GoogleCloud["Google Cloud / OpenRouter"]
        GatewayRouter -.->|Fallback Route| LMStudio["LM Studio (Port 1234)"]
    end
```

### Flow Breakdown:
1. **Extraction**: `bootstrap.py` asks Graphify to scan your project folders.
2. **Routing**: Graphify hits port `8666` (the Hermes Gateway). The Gateway pushes the request to the Cloud for speed, but seamlessly falls back to LM Studio if there's a hiccup.
3. **Ingestion**: Graphify dumps the parsed AST and semantic data into `graph.json`. Our Indexer sucks that JSON up and stores it in the local SQLite `graph.db`.
4. **Analysis**: You ask Roo-code a question. Roo-code queries the local `graph.db` database and gives you the answer without ever needing to re-read your massive source code files.
