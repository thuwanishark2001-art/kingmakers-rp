using Unity.Netcode.Components;

namespace Kingmakers.Net
{
    /// <summary>
    /// The owning client moves the object and the server relays it. Keeps walking and
    /// driving responsive on mobile networks. Server-side movement checks are on the roadmap.
    /// </summary>
    public class OwnerNetworkTransform : NetworkTransform
    {
        protected override bool OnIsServerAuthoritative() => false;
    }
}
