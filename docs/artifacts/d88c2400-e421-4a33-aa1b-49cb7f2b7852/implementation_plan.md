# SIA Digital Portal — Implementation Plan

This plan covers the transition from a static prototype to a version-controlled, audit-capable infrastructure management system.

## Proposed Changes

### 🏛️ Core Architecture
- **Tech Stack**: React 19, Vite 8, Tailwind v4, Leaflet.
- **State Management**: Centralized `mockData.js` evolving into a structured schema with relational integrity.

### 🗺️ GIS & Location (WebGIS Integration)
- **Parcel Persistence**: Implement a `location_history` array for every project.
- **Cadastral Updates**: Ensure that when a parcel number changes (e.g., land subdivision), the project preserves its geographic "work area" and audit trail.

### 📄 Protocol & Templates (System of Record)
- **Standardized Templates**: Create a repository of contract definitions, BoQs (Bill of Quantities), and technical specs.
- **Schema Versioning**: Use a versioning system (v1.0, v1.1) for templates so that historical projects are tied to the definitions that existed at their creation.

### 🤖 AI-Accelerated Development
- **Gemini Offloading**: Use Gemini to generate boilerplate for complex admin forms and regulatory document parsers.
- **AI Studio Tuning**: Use AI Studio to generate specific JSON schemas for Seychelles-specific infrastructure standards.

## Phase 8: Backend API & Data Persistence

### User Review Required
> [!IMPORTANT]
> **Database Architecture Decision Needed**: We need to decide whether to use a pure **PostgreSQL** relational database using an ORM like Prisma (strict schema, excellent for the GIS data via PostGIS), or a **Hybrid NoSQL (MongoDB)** approach for flexible document/template storage. 
> I recommend PostgreSQL with PostGIS because of our heavy reliance on geospatial coordinates, bounding boxes, and strict RBAC schemas. Do you approve PostgreSQL + Prisma for the Express.js API?

### Proposed Changes
#### Server Architecture
- **Framework**: Express.js (Node 20.x).
- **ORM**: Prisma (or Mongoose if Hybrid is chosen).
- **Authentication**: JWT-based token auth replacing the frontend mock context.
- **Storage**: Set up Multer for local/S3 real file uploads.

#### Migration Path
1. Scaffold Express server in `sia-portal/api/`.
2. Define `Project`, `User`, `Task`, `Document`, and `Comment` models based strictly on our normalized mockData schema.
3. Replace Vite frontend `mockData.js` imports with `tanstack/react-query` or standard `fetch` hooks targeting the new API.

## Verification Plan

### Automated Tests
- **Consistency Tests**: Verify that changing a parcel ID doesn't orphan the project data.
- **Schema Validation**: Ensure documents match the required template version.

### Manual Verification
- **Drill-down Check**: Click every "Full Detail" button across Dashboard and Map to verify routing.
- **Historical Toggle**: Verify that old project locations show up correctly on the map when toggled.
