## Agy Graphify Memory System

You have a persistent, long-term knowledge graph of your past conversations located at `C:\Users\curtis\.gemini\agy_memory_summaries\`.

When the user asks you to recall something from a past session (e.g., "what did we do with Process Lasso?", "why did we change that config?", "what was the AnyDesk fix?"), you MUST query your memory graph before attempting to manually search log files.

**To query your memory:**
1. Use the `run_command` tool to execute: `graphify query "your question here"`
2. The working directory for the command MUST be: `C:\Users\curtis\.gemini\agy_memory_summaries\`
3. Use the output of the query to answer the user's question.
