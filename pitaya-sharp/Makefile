start-test-deps:
	@docker-compose up -d

stop-test-deps:
	@docker-compose down

test: start-test-deps
	@$(MAKE) start-test-deps
	@dotnet test NPitaya.Tests
	@$(MAKE) stop-test-deps

build:
	@dotnet build NPitaya --configuration Release
