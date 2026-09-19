FROM postgres:latest

# Create the database schema on first startup.
COPY database/init.sql /docker-entrypoint-initdb.d/1.sql
