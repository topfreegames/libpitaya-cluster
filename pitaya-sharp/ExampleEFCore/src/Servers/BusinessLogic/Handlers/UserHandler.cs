using System;
using System.Linq;
using ExampleORM.Models;
using Gen.Protos;
using NPitaya;
using NPitaya.Models;
using Session = Protos.Session;
using User = Gen.Protos.User;

namespace ExampleORM.Servers.BusinessLogic.Handlers
{
    public class UserHandler: BaseHandler
    {
        private User ModelsUserToProtosUser(Models.User modelsUser)
        {
            return new User
            {
                Id = modelsUser.Id.ToString(),
                Name = modelsUser.Name,
                Token = modelsUser.Token.ToString()
            };
        }

        public User Authenticate(PitayaSession session, AuthenticateArgs args)
        {
            using (var context = new ExampleContext())
            {
                if (args.Token.Length > 0)
                {
                    var user = context.Users.Single(u => u.Token == Guid.Parse(args.Token));
                    // if user is found, return it!
                    if (user != null)
                    {
                        return ModelsUserToProtosUser(user);
                    }

                    throw new PitayaException("user not found!");
                }

                // if token was not sent, create an user and return!
                var newUser = new Models.User();
                newUser = context.Users.Add(newUser).Entity;
                context.SaveChanges();
                return ModelsUserToProtosUser(newUser);
            }
        }

        public Answer ChangeName(Session session, SimpleArg arg)
        {
            return null;
        }
    }
}