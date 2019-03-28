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
        private Models.User GetUserFromToken(Guid token, ExampleContext ctx)
        {
            return ctx.Users.Single(u => u.Token == token);
        }
        
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
                    return ModelsUserToProtosUser(GetUserFromToken(new Guid(args.Token), context));
                }

                // if token was not sent, create an user and return!
                var newUser = new Models.User();
                newUser = context.Users.Add(newUser).Entity;
                context.SaveChanges();
                return ModelsUserToProtosUser(newUser);
            }
        }

        public Answer ChangeName(PitayaSession session, ChangeNameArgs arg)
        {
            using (var context = new ExampleContext())
            {
                var userModel = GetUserFromToken(new Guid(arg.Token), context);
                userModel.Name = arg.Name;
                context.Update(userModel);
                context.SaveChanges();
                return new Answer
                {
                    Code = "OK!"
                };
            }
        }
    }
}