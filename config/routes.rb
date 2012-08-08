HyderabadHomes::Application.routes.draw do

  devise_for :administrators , :controllers => {:sessions => "administrators/sessions",:registrations => "administrators/registrations"}

  namespace :administrators do
    resources :dashboard
    resources :properties
    resources :ownership_types
    resources :looking_fors
  end

    get "home/index"
    post "home/search_properties"
    get "home/search_properties"

    post "home/add_to_cart"
    get "home/add_to_cart"
    get "home/add_to_favourites"
    post "home/create_properties"

    get "home/about_us"
    get "home/contact_us"

  devise_for :users  , :controllers => {:sessions => "users/sessions",:registrations => "users/registrations", :confirmations => "users/confirmations" }

=begin
 devise_for :users do
    match "Logout", :to => "users/sessions#destroy"
  end
=end

  namespace :users do
    resources :dashboard do
      get "invite_friends", :on => :collection
      post "import" , :on => :collection
      post "create_contacts" , :on => :collection
    end
    resources :profiles
    resources :user_properties do
      post "search_properties_user", :on => :collection
      #get "add_to_cart" , :on => :collection
    end
    resources :user_looking_for_properties

  end

  # The priority is based upon order of creation:
  # first created -> highest priority.

  # Sample of regular route:
  #   match 'products/:id' => 'catalog#view'
  # Keep in mind you can assign values other than :controller and :action

  # Sample of named route:
  #   match 'products/:id/purchase' => 'catalog#purchase', :as => :purchase
  # This route can be invoked with purchase_url(:id => product.id)

  # Sample resource route (maps HTTP verbs to controller actions automatically):
  #   resources :products

  # Sample resource route with options:
  #   resources :products do
  #     member do
  #       get 'short'
  #       post 'toggle'
  #     end
  #
  #     collection do
  #       get 'sold'
  #     end
  #   end

  # Sample resource route with sub-resources:
  #   resources :products do
  #     resources :comments, :sales
  #     resource :seller
  #   end

  # Sample resource route with more complex sub-resources
  #   resources :products do
  #     resources :comments
  #     resources :sales do
  #       get 'recent', :on => :collection
  #     end
  #   end

  # Sample resource route within a namespace:
  #   namespace :admin do
  #     # Directs /admin/products/* to Admin::ProductsController
  #     # (app/controllers/admin/products_controller.rb)
  #     resources :products
  #   end

  # You can have the root of your site routed with "root"
  # just remember to delete public/index.html.
  root :to => 'home#index'

  # See how all your routes lay out with "rake routes"

  # This is a legacy wild controller route that's not recommended for RESTful applications.
  # Note: This route will make all actions in every controller accessible via GET requests.
  # match ':controller(/:action(/:id))(.:format)'
end
